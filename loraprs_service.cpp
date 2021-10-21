#include "loraprs_service.h"

namespace LoraPrs {

byte Service::rxBuf_[256];

#ifdef USE_RADIOLIB
bool Service::interruptEnabled_ = true;
std::shared_ptr<SX1278> Service::radio_;
#endif

Service::Service()
  : Kiss::Processor()
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , csmaSlotTimePrev_(0)
  , serialBt_()
  , serialBLE_()
  , kissServer_(new WiFiServer(CfgKissPort))
{
#ifdef USE_RADIOLIB
  interruptEnabled_ = true;
#endif
}

void Service::setup(const Config &conf)
{
  config_ = conf;  
  previousBeaconMs_ = 0;

  ownCallsign_ = AX25::Callsign(config_.AprsLogin);
  if (!ownCallsign_.IsValid()) {
    Serial.println("Own callsign is not valid");
  }

  aprsLoginCommand_ = String("user ") + config_.AprsLogin + String(" pass ") + 
    config_.AprsPass + String(" vers ") + CfgLoraprsVersion;
  if (config_.EnableIsToRf && config_.AprsFilter.length() > 0) {
    aprsLoginCommand_ += String(" filter ") + config_.AprsFilter;
  }
  aprsLoginCommand_ += String("\n");

  // peripherals
  setupLora(config_.LoraFreq, config_.LoraBw, config_.LoraSf, 
    config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, config_.LoraEnableCrc);

  if (needsWifi()) {
    setupWifi(config_.WifiSsid, config_.WifiKey);
  }

  if (needsBt() || config_.BtName.length() > 0) {
    setupBt(config_.BtName);
  }

  if (needsAprsis() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }

  if (config_.PttEnable) {
    Serial.println("External PTT is enabled");
    pinMode(config_.PttPin, OUTPUT);
  }
}

void Service::setupWifi(const String &wifiName, const String &wifiKey)
{
  Serial.print("WIFI connecting to " + wifiName);

  WiFi.setHostname("loraprs");
  if (config_.WifiEnableAp) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiName.c_str(), wifiKey.c_str());
  
    int retryCnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(CfgConnRetryMs);
      Serial.print(".");
      if (retryCnt++ >= CfgConnRetryMaxTimes) {
        Serial.println("failed");
        return;
      }
    }
    Serial.println("ok");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.softAP(wifiName.c_str(), wifiKey.c_str());    
    Serial.println(WiFi.softAPIP());
  }
  if (config_.KissEnableTcpIp) {
    kissServer_->begin();
  }
}

void Service::reconnectWifi() const
{
  Serial.print("WIFI re-connecting...");

  int retryCnt = 0;
  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(CfgConnRetryMs);
    Serial.print(".");
    if (retryCnt++ >= CfgConnRetryMaxTimes) {
      Serial.println("failed");
      return;
    }
  }

  Serial.println("ok");
  Serial.println(WiFi.localIP());
  
  if (config_.KissEnableTcpIp) {
    kissServer_->begin();
  }
}

bool Service::reconnectAprsis()
{
  Serial.print("APRSIS connecting...");
  
  if (!aprsisConn_.connect(config_.AprsHost.c_str(), config_.AprsPort)) {
    Serial.println("Failed to connect to " + config_.AprsHost + ":" + config_.AprsPort);
    return false;
  }
  Serial.println("ok");

  aprsisConn_.print(aprsLoginCommand_);
  return true;
}

void Service::setupLora(long loraFreq, long bw, int sf, int cr, int pwr, int sync, bool enableCrc)
{
  Serial.print("LoRa init: ");
  Serial.print(loraFreq); Serial.print(", ");
  Serial.print(bw); Serial.print(", ");
  Serial.print(sf); Serial.print(", ");
  Serial.print(cr); Serial.print(", ");
  Serial.print(pwr); Serial.print(", ");
  Serial.print(sync, 16); Serial.print(", ");
  Serial.print(enableCrc); Serial.print("...");

  isImplicitHeaderMode_ = sf == 6;

#ifdef USE_RADIOLIB
  radio_ = std::make_shared<SX1278>(new Module(config_.LoraPinSs, config_.LoraPinDio0, config_.LoraPinRst, RADIOLIB_NC));
  int state = radio_->begin((float)loraFreq / 1e6, (float)bw / 1e3, sf, cr, sync, pwr);
  if (state != ERR_NONE) {
    Serial.print("Radio start error: "); Serial.println(state);
  }
  radio_->setCRC(enableCrc);
  //radio_->forceLDRO(false);
  //radio_->setRfSwitchPins(4, 5);

  radio_->clearDio0Action();
  radio_->setDio0Action(onLoraDataAvailableIsr);

  state = radio_->startReceive();
  if (state != ERR_NONE) {
    Serial.print("Receive start error: "); Serial.println(state);
  }
  
#else // USE_RADIOLIB

  LoRa.setPins(config_.LoraPinSs, config_.LoraPinRst, config_.LoraPinDio0);

  int retryCnt = 0;
  while (!LoRa.begin(loraFreq)) {
    Serial.print(".");
    delay(CfgConnRetryMs);
    if (retryCnt++ >= CfgConnRetryMaxTimes) {
      Serial.println("failed");
      return;
    }
  }
  LoRa.setSyncWord(sync);
  LoRa.setSpreadingFactor(sf);
  LoRa.setSignalBandwidth(bw);
  LoRa.setCodingRate4(cr);
  LoRa.setTxPower(pwr);

  if (enableCrc) {
    LoRa.enableCrc();
  }

  if (config_.LoraUseIsr) {
    LoRa.onReceive(onLoraDataAvailableIsr);
    LoRa.receive();
  }
#endif // USE_RADIOLIB

  Serial.println("ok");
}

void Service::setupBt(const String &btName)
{
  String btType = config_.BtEnableBle ? "BLE" : "BT";
  Serial.print(btType + " init " + btName + "...");
  
  bool btOk = config_.BtEnableBle 
    ? serialBLE_.begin(btName.c_str()) 
    : serialBt_.begin(btName);
  
  if (btOk) {
    Serial.println("ok");
  }
  else {
    Serial.println("failed");
  }
}

void Service::loop()
{
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConn_.connected() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }

  // RX path, Rig -> Serial
  bool isRigToSerialProcessed = false;

#ifdef USE_RADIOLIB
  isRigToSerialProcessed = processRigToSerial();
#else
  if (config_.LoraUseIsr) {
    isRigToSerialProcessed = processRigToSerial();
  } else {
     if (int packetSize = LoRa.parsePacket()) {
        loraReceive(packetSize);
        isRigToSerialProcessed = true;
     }
  }
#endif

  // TX path, Serial -> Rig
  if (!isRigToSerialProcessed) {

    long currentTime = millis();
    if (!isLoraRxBusy() && currentTime > csmaSlotTimePrev_ + csmaSlotTime_ && random(0, 255) < csmaP_) {
      if (aprsisConn_.available() > 0) {
        onAprsisDataAvailable();
      }
      if (needsBeacon()) {
        sendPeriodicBeacon();
      }
      bool allTxProcessed = processSerialToRig();
      if (allTxProcessed) {
#ifdef USE_RADIOLIB
        int state = radio_->startReceive();
        if (state != ERR_NONE) {
          Serial.print("Start receive error: "); Serial.println(state);
        }
#else
        if (config_.LoraUseIsr) {
          LoRa.receive();
        }
#endif
      }
      csmaSlotTimePrev_ = currentTime;
    }
  }
  delay(CfgPollDelayMs);
}

bool Service::isLoraRxBusy() {
#ifdef USE_RADIOLIB
  return cfg_.LoraUseCad && (radio_->getModemStatus() & 0x01); // SX1278_STATUS_SIG_DETECT
#else
  return false;
#endif
}

#ifdef USE_RADIOLIB

ICACHE_RAM_ATTR void Service::onLoraDataAvailableIsr() {
  if (interruptEnabled_) {
    int packetSize = radio_->getPacketLength();
  
    if (packetSize > 0) {
      
      int state = radio_->readData(rxBuf_, packetSize);
      if (state == ERR_NONE) {
        queueRigToSerialIsr(Cmd::Data, rxBuf_, packetSize);
      } else {
        Serial.print("Read data error: "); Serial.println(state);
      }
      
      state = radio_->startReceive();
      if (state != ERR_NONE) {
        Serial.print("Start receive error: "); Serial.println(state);
      }
    }
  }
}

#else // USE_RADIOLIB

ICACHE_RAM_ATTR void Service::onLoraDataAvailableIsr(int packetSize)
{
  int rxBufIndex = 0;

  for (int i = 0; i < packetSize; i++) {
    rxBuf_[rxBufIndex++] = LoRa.read();
  }
  queueRigToSerialIsr(Cmd::Data, rxBuf_, rxBufIndex);
}

#endif // USE_RADIOLIB

void Service::sendPeriodicBeacon()
{
  long currentMs = millis();

  if (previousBeaconMs_ == 0 || currentMs - previousBeaconMs_ >= config_.AprsRawBeaconPeriodMinutes * 60 * 1000) {
      AX25::Payload payload(config_.AprsRawBeacon);
      if (payload.IsValid()) {
        sendAX25ToLora(payload);
        if (config_.EnableRfToIs) {
          sendToAprsis(payload.ToString());
        }
        Serial.println("Periodic beacon is sent");
      }
      else {
        Serial.println("Beacon payload is invalid");
      }
      previousBeaconMs_ = currentMs;
  }
}

void Service::sendToAprsis(const String &aprsMessage)
{
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConn_.connected()) {
    reconnectAprsis();
  }
  aprsisConn_.println(aprsMessage);

  if (!config_.EnablePersistentAprsConnection) {
    aprsisConn_.stop();
  }
}

void Service::onAprsisDataAvailable()
{
  String aprsisData;

  while (aprsisConn_.available() > 0) {
    char c = aprsisConn_.read();
    if (c == '\r') continue;
    Serial.print(c);
    if (c == '\n') break;
    aprsisData += c;
    if (aprsisData.length() >= CfgMaxAprsInMessageSize) {
      Serial.println("APRS-IS incoming message is too long, skipping tail");
      break;
    }
  }

  if (config_.EnableIsToRf && aprsisData.length() > 0) {
    AX25::Payload payload(aprsisData);
    if (payload.IsValid()) {
      sendAX25ToLora(payload);
    }
    else {
      Serial.println("Unknown payload from APRSIS, ignoring");
    }
  }
}

void Service::sendSignalReportEvent(int rssi, float snr)
{
  struct SignalReport signalReport;

  signalReport.rssi = htobe16(rssi);
  signalReport.snr = htobe16(snr * 100);

  sendRigToSerial(Cmd::SignalReport, (const byte *)&signalReport, sizeof(SignalReport));
}

bool Service::sendAX25ToLora(const AX25::Payload &payload)
{
  byte buf[CfgMaxAX25PayloadSize];
  int bytesWritten = payload.ToBinary(buf, sizeof(buf));
  if (bytesWritten <= 0) {
    Serial.println("Failed to serialize payload");
    return false;
  }
  queueSerialToRig(Cmd::Data, buf, bytesWritten);
  return true;
}

void Service::onRigPacket(void *packet, int packetLength)
{  
#ifdef USE_RADIOLIB
  long frequencyErrorHz = radio_->getFrequencyError();
#else
  long frequencyErrorHz = LoRa.packetFrequencyError();
#endif
  if (config_.EnableAutoFreqCorrection && abs(frequencyErrorHz) > config_.AutoFreqCorrectionDeltaHz) {
    config_.LoraFreq -= frequencyErrorHz;
    Serial.print("Correcting frequency: "); Serial.println(frequencyErrorHz);
#ifdef USE_RADIOLIB
    radio_->setFrequency((float)config_.LoraFreq / 1e6);
    int state = radio_->startReceive();
    if (state != ERR_NONE) {
      Serial.print("Start receive error: "); Serial.println(state);
    }
#else
    LoRa.setFrequency(config_.LoraFreq);
    if (config_.LoraUseIsr) {
      LoRa.idle();
      LoRa.receive();
    }
#endif
  }

  if (config_.KissEnableExtensions) {
#ifdef USE_RADIOLIB
    sendSignalReportEvent(radio_->getRSSI(), radio_->getSNR());
#else
    sendSignalReportEvent(LoRa.packetRssi(), LoRa.packetSnr());
#endif
  }

  if (!config_.IsClientMode) {
    processIncomingRawPacketAsServer((const byte*)packet, packetLength);
  }
}

#ifndef USE_RADIOLIB
void Service::loraReceive(int packetSize)
{
  int rxBufIndex = 0;
  byte rxBuf[packetSize];
  
  while (LoRa.available()) {
    rxBuf[rxBufIndex++] = LoRa.read();
  }
  sendRigToSerial(Cmd::Data, rxBuf, rxBufIndex);
  onRigPacket(rxBuf, rxBufIndex);
}
#endif

void Service::processIncomingRawPacketAsServer(const byte *packet, int packetLength) {

  AX25::Payload payload(packet, packetLength);

  if (payload.IsValid()) {

#ifdef USE_RADIOLIB
    float snr = radio_->getSNR();
    int rssi = radio_->getRSSI();
    long frequencyError = radio_->getFrequencyError();
#else
    float snr = LoRa.packetSnr();
    int rssi = LoRa.packetRssi();
    long frequencyError = LoRa.packetFrequencyError();
#endif
    String signalReport = String(" ") +
      String("rssi: ") +
      String(snr < 0 ? rssi + snr : rssi) +
      String("dBm, ") +
      String("snr: ") +
      String(snr) +
      String("dB, ") +
      String("err: ") +
      String(frequencyError) +
      String("Hz");
    
    String textPayload = payload.ToString(config_.EnableSignalReport ? signalReport : String());
    Serial.println(textPayload);

    if (config_.EnableRfToIs) {
      sendToAprsis(textPayload);
      Serial.println("Packet sent to APRS-IS");
    }
    if (config_.EnableRepeater && payload.Digirepeat(ownCallsign_)) {
      sendAX25ToLora(payload);
      Serial.println("Packet digirepeated");
    }
  } else {
    Serial.println("Skipping non-AX25 payload");
  }
}

bool Service::onRigTxBegin()
{
  if (config_.PttEnable) {
    digitalWrite(config_.PttPin, HIGH);
    delay(config_.PttTxDelayMs);
  } else {
    delay(CfgPollDelayMs);
  }
#ifdef USE_RADIOLIB
  return true;
#else
  return (LoRa.beginPacket(isImplicitHeaderMode_) == 1);
#endif
}

void Service::onRigTx(byte b)
{
#ifdef USE_RADIOLIB
  txQueue_.push(b);
#else
  LoRa.write(b);
#endif
}

void Service::onRigTxEnd()
{
#ifdef USE_RADIOLIB
  int txPacketSize = txQueue_.size();
  byte txBuf[txPacketSize];
  
  for (int i = 0; i < txPacketSize; i++) {
    txBuf[i] = txQueue_.shift();
  }

  interruptEnabled_ = false;
  int state = radio_->transmit(txBuf, txPacketSize);
  if (state != ERR_NONE) {
    Serial.print("TX error: "); Serial.println(state);
  }
  interruptEnabled_ = true;
#endif

  if (config_.PttEnable) {
#ifndef USE_RADIOLIB
    LoRa.endPacket(false);
#endif
    delay(config_.PttTxTailMs);
    digitalWrite(config_.PttPin, LOW);
  } else {
#ifndef USE_RADIOLIB
    LoRa.endPacket(true);
#endif
  }
}

WiFiClient Service::getClient() 
{
  if (config_.KissEnableTcpIp) {
    WiFiClient client = kissServer_->available();
    if (client) {

      if (client.connected()) {
        Serial.println("Connected to client");
      }  
      return client;
    }
  }
  return 0;
}

void Service::onSerialTx(byte b)
{
  WiFiClient client = getClient();
  if (client) {
    client.write(b);
  }
  else if (config_.BtEnableBle) {
    serialBLE_.write(b);
  }
  else {
    serialBt_.write(b);
  }
}

bool Service::onSerialRxHasData()
{
  WiFiClient client = getClient();
  if (client) {
    return client.available();
  }
  else if (config_.BtEnableBle) {
    return serialBLE_.available();
  }
  else {
    return serialBt_.available();
  }
}

bool Service::onSerialRx(byte *b)
{
  int rxResult;
  
  WiFiClient client = getClient();
  if (client) {
    rxResult = client.read();
  }
  else {
    rxResult = config_.BtEnableBle 
      ? serialBLE_.read() 
      : serialBt_.read();
  }
  if (rxResult == -1) {
    return false;
  }
  *b = (byte)rxResult;
  return true;
}

void Service::onControlCommand(Cmd cmd, byte value)
{
  switch (cmd) {
    case Cmd::P:
      Serial.print("CSMA P: "); Serial.println(value);
      csmaP_ = value;
      break;
    case Cmd::SlotTime:
      Serial.print("CSMA SlotTime: "); Serial.println(value);
      csmaSlotTime_ = (long)value * 10;
      break;
    case Cmd::TxDelay:
      Serial.print("TX delay: "); Serial.println(value);
      config_.PttTxDelayMs = (long)value * 10;
      break;
    case Cmd::TxTail:
      Serial.print("TX tail: "); Serial.println(value);
      config_.PttTxTailMs = (long)value * 10;
      break;
    default:
      break;
  }
}

void Service::onRadioControlCommand(const std::vector<byte> &rawCommand) {

  if (config_.KissEnableExtensions && rawCommand.size() == sizeof(SetHardware)) {
    const struct SetHardware * setHardware = reinterpret_cast<const struct SetHardware*>(rawCommand.data());
    
    config_.LoraFreq = be32toh(setHardware->freq);
    config_.LoraBw = be32toh(setHardware->bw);
    config_.LoraSf = be16toh(setHardware->sf);
    config_.LoraCodingRate = be16toh(setHardware->cr);
    config_.LoraPower = be16toh(setHardware->pwr);
    config_.LoraSync = be16toh(setHardware->sync);
    config_.LoraEnableCrc = setHardware->crc;

    setupLora(config_.LoraFreq, config_.LoraBw, config_.LoraSf, 
      config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, config_.LoraEnableCrc);
  } else {
    Serial.println("Radio control command of wrong size");
  }
}

void Service::onRebootCommand()
{
  Serial.println("Reboot requested");
  ESP.restart();
}

} // LoraPrs
