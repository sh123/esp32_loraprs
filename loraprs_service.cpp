#include "loraprs_service.h"

namespace LoraPrs {

byte Service::rxBuf_[256];

#ifdef USE_RADIOLIB
#pragma message("Using RadioLib")
bool Service::interruptEnabled_ = true;
std::shared_ptr<MODULE_NAME> Service::radio_;
#else
#pragma message("Using arduino-LoRa")
#endif

Service::Service()
  : Kiss::Processor()
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , csmaSlotTimePrev_(0)
  , serialBt_()
  , serialBLE_()
  , kissServer_(new WiFiServer(CfgKissPort))
  , isKissConn_(false)
{
#ifdef USE_RADIOLIB
  interruptEnabled_ = true;
#endif
}

void Service::setup(const Config &conf)
{
  config_ = conf;  
  previousBeaconMs_ = 0;

  LOG_SET_OPTION(false, false, true);  // disable file, line, enable func

  // disable logging when USB is used for data transfer
  if (config_.UsbSerialEnable) {
    LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);
  }
#ifdef USE_RADIOLIB
  LOG_INFO("Built with RadioLib library");
#else
  LOG_INFO("Built with arduino-LoRa library");
#endif

  ownCallsign_ = AX25::Callsign(config_.AprsLogin);
  if (!ownCallsign_.IsValid()) {
    LOG_ERROR("Own callsign", config_.AprsLogin, "is not valid");
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

  if (needsBt()) {
    setupBt(config_.BtName);
  }

  if (needsAprsis() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }

  if (config_.PttEnable) {
    LOG_INFO("External PTT is enabled");
    pinMode(config_.PttPin, OUTPUT);
  }
}

void Service::setupWifi(const String &wifiName, const String &wifiKey)
{
  WiFi.setHostname("loraprs");

  // AP mode
  if (config_.WifiEnableAp) {
    LOG_INFO("WIFI is running in AP mode", wifiName);
    WiFi.softAP(wifiName.c_str(), wifiKey.c_str());    
    LOG_INFO("IP address:", WiFi.softAPIP());

  // Client/STA mode
  } else {
    LOG_INFO("WIFI connecting to", wifiName);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiName.c_str(), wifiKey.c_str());
  
    int retryCnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(CfgConnRetryMs);
      LOG_WARN("WIFI retrying", retryCnt);
      if (retryCnt++ >= CfgConnRetryMaxTimes) {
        LOG_ERROR("WIFI connect failed");
        return;
      }
    }
    LOG_INFO("WIFI connected to", wifiName);
    LOG_INFO("IP address:", WiFi.localIP());
  }
  // Run KISS server if enabled
  if (config_.KissEnableTcpIp) {
    LOG_INFO("KISS TCP/IP server started on port", CfgKissPort);
    kissServer_->begin();
  }
}

void Service::reconnectWifi() const
{
  // AP mode does not require re-connection
  if (config_.WifiEnableAp) return;
  
  LOG_WARN("WIFI re-connecting...");

  int retryCnt = 0;
  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(CfgConnRetryMs);
    LOG_WARN("WIFI re-connecting", retryCnt);
    if (retryCnt++ >= CfgConnRetryMaxTimes) {
      LOG_ERROR("WIFI re-connect failed");
      return;
    }
  }

  LOG_INFO("WIFI reconnected, IP address", WiFi.localIP());
  
  if (config_.KissEnableTcpIp) {
    LOG_INFO("KISS TCP/IP server started on port", CfgKissPort);
    kissServer_->begin();
  }
}

bool Service::reconnectAprsis()
{
  LOG_INFO("APRSIS connecting to", config_.AprsHost);
  
  if (!aprsisConn_.connect(config_.AprsHost.c_str(), config_.AprsPort)) {
    LOG_ERROR("Failed to connect to", config_.AprsHost, ":", config_.AprsPort);
    return false;
  }
  LOG_INFO("APRSIS connected");
  aprsisConn_.print(aprsLoginCommand_);
  LOG_INFO("APRSIS logged in");
  return true;
}

void Service::setupLora(long loraFreq, long bw, int sf, int cr, int pwr, int sync, bool enableCrc)
{
  LOG_INFO("Initializing LoRa");
  LOG_INFO("Frequency:", loraFreq, "Hz");
  LOG_INFO("Bandwidth:", bw, "Hz");
  LOG_INFO("Spreading:", sf);
  LOG_INFO("Coding rate:", cr);
  LOG_INFO("Power:", pwr, "dBm");
  LOG_INFO("Sync:", "0x" + String(sync, 16));
  LOG_INFO("CRC:", enableCrc ? "enabled" : "disabled");
  
  isImplicitHeaderMode_ = sf == 6;

#ifdef USE_RADIOLIB
  radio_ = std::make_shared<MODULE_NAME>(new Module(config_.LoraPinSs, config_.LoraPinDio0, config_.LoraPinRst, RADIOLIB_NC));
  int state = radio_->begin((float)loraFreq / 1e6, (float)bw / 1e3, sf, cr, sync, pwr);
  if (state != ERR_NONE) {
    LOG_ERROR("Radio start error:", state);
  }
  radio_->setCRC(enableCrc);
  //radio_->forceLDRO(false);
  //radio_->setRfSwitchPins(4, 5);

#if MODULE_NAME == SX1268
  radio_->clearDio1Action();
  radio_->setDio1Action(onLoraDataAvailableIsr);
#else
  radio_->clearDio0Action();
  radio_->setDio0Action(onLoraDataAvailableIsr);
#endif
  state = radio_->startReceive();
  if (state != ERR_NONE) {
    LOG_ERROR("Receive start error:", state);
  }
  
#else // USE_RADIOLIB

  LoRa.setPins(config_.LoraPinSs, config_.LoraPinRst, config_.LoraPinDio0);

  int retryCnt = 0;
  while (!LoRa.begin(loraFreq)) {
    LOG_WARN("LoRa init retry", retryCnt);
    delay(CfgConnRetryMs);
    if (retryCnt++ >= CfgConnRetryMaxTimes) {
      LOG_ERROR("LoRa init failed");
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

  LOG_INFO("LoRa initialized");
}

void Service::setupBt(const String &btName)
{
  String btType = config_.BtEnableBle ? "BLE" : "BT";
  LOG_INFO(btType, "init", btName);
  
  bool btOk = config_.BtEnableBle 
    ? serialBLE_.begin(btName.c_str()) 
    : serialBt_.begin(btName);
  
  if (btOk) {
    LOG_INFO(btType, "initialized");
  }
  else {
    LOG_ERROR(btType, "failed");
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
  if (config_.KissEnableTcpIp) {
    attachKissNetworkClient();
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
          LOG_ERROR("Start receive error: ", state);
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
  return config_.LoraUseCad && (radio_->getModemStatus() & 0x01); // SX1278_STATUS_SIG_DETECT
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
        LOG_ERROR("Read data error: ", state);
      }
      
      state = radio_->startReceive();
      if (state != ERR_NONE) {
        LOG_ERROR("Start receive error: ", state);
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
        LOG_INFO("Periodic beacon is sent");
      }
      else {
        LOG_ERROR("Beacon payload is invalid");
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
    if (c == '\n') break;
    aprsisData += c;
    if (aprsisData.length() >= CfgMaxAprsInMessageSize) {
      LOG_WARN("APRS-IS incoming message is too long, skipping tail");
      break;
    }
  }

  LOG_INFO(aprsisData);
  
  if (config_.EnableIsToRf && aprsisData.length() > 0) {
    AX25::Payload payload(aprsisData);
    if (payload.IsValid()) {
      sendAX25ToLora(payload);
    }
    else {
      LOG_WARN("Unknown payload from APRSIS, ignoring");
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
    LOG_WARN("Failed to serialize payload");
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
    LOG_INFO("Correcting frequency:", frequencyErrorHz);
#ifdef USE_RADIOLIB
    radio_->setFrequency((float)config_.LoraFreq / 1e6);
    int state = radio_->startReceive();
    if (state != ERR_NONE) {
      LOG_ERROR("Start receive error:", state);
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
    LOG_INFO(textPayload);

    if (config_.EnableRfToIs) {
      sendToAprsis(textPayload);
      LOG_INFO("Packet sent to APRS-IS");
    }
    if (config_.EnableRepeater && payload.Digirepeat(ownCallsign_)) {
      sendAX25ToLora(payload);
      LOG_INFO("Packet digirepeated");
    }
  } else {
    LOG_WARN("Skipping non-AX25 payload");
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
    LOG_ERROR("TX error: ", state);
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

void Service::attachKissNetworkClient() 
{
  // connected, client dropped off
  if (isKissConn_) {
    if (!kissConn_.connected()) {
      LOG_INFO("KISS TCP/IP client disconnected");
      isKissConn_ = false;
      kissConn_.stop();
    }
  }
  WiFiClient wifiClient = kissServer_->available();
  // new client connected
  if (wifiClient && wifiClient.connected()) {
    // drop off current one
    if (isKissConn_) {
      kissConn_.stop();
    }
    LOG_INFO("New KISS TCP/IP client connected");
    kissConn_ = wifiClient;
    isKissConn_ = true;
  }
}

void Service::onSerialTx(byte b)
{
  if (config_.UsbSerialEnable) {
    Serial.write(b);
  } 
  else if (isKissConn_) {
    kissConn_.write(b);
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
  if (config_.UsbSerialEnable) {
    return Serial.available();
  } 
  else if (isKissConn_) {
    return kissConn_.available();
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

  if (config_.UsbSerialEnable) {
    rxResult = Serial.read();
  } 
  else if (isKissConn_) {
    rxResult = kissConn_.read();
    // client dropped off
    if (rxResult == -1) {
      kissConn_.stop();
      isKissConn_ = false;
    }
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
      LOG_INFO("CSMA P:", value);
      csmaP_ = value;
      break;
    case Cmd::SlotTime:
      LOG_INFO("CSMA SlotTime:", value);
      csmaSlotTime_ = (long)value * 10;
      break;
    case Cmd::TxDelay:
      LOG_INFO("TX delay:", value);
      config_.PttTxDelayMs = (long)value * 10;
      break;
    case Cmd::TxTail:
      LOG_INFO("TX tail:", value);
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
    LOG_ERROR("Radio control command of wrong size");
  }
}

void Service::onRebootCommand()
{
  LOG_INFO("Reboot requested");
  ESP.restart();
}

} // LoraPrs
