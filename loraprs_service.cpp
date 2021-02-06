#include "loraprs_service.h"

namespace LoraPrs {
  
Service::Service()
  : Kiss::Processor()
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , csmaSlotTimePrev_(0)
  , serialBt_()
{
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
  if (config_.AprsFilter.length() > 0) {
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
}

void Service::setupWifi(const String &wifiName, const String &wifiKey)
{
  if (!config_.IsClientMode) {
    Serial.print("WIFI connecting to " + wifiName);

    WiFi.setHostname("loraprs");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiName.c_str(), wifiKey.c_str());

    int retryCnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(CfgConnRetryMs);
      Serial.print(".");
      if (retryCnt++ >= CfgWiFiConnRetryMaxTimes) {
        Serial.println("failed");
        return;
      }
    }
    Serial.println("ok");
    Serial.println(WiFi.localIP());
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
    if (retryCnt++ >= CfgWiFiConnRetryMaxTimes) {
      Serial.println("failed");
      return;
    }
  }

  Serial.println("ok");
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
  
  LoRa.setPins(config_.LoraPinSs, config_.LoraPinRst, config_.LoraPinDio0);
  
  while (!LoRa.begin(loraFreq)) {
    Serial.print(".");
    delay(CfgConnRetryMs);
  }
  LoRa.setSyncWord(sync);
  LoRa.setSpreadingFactor(sf);
  LoRa.setSignalBandwidth(bw);
  LoRa.setCodingRate4(cr);
  LoRa.setTxPower(pwr);
  if (enableCrc) {
    LoRa.enableCrc();
  }
  
  Serial.println("ok");  
}

void Service::setupBt(const String &btName)
{
  Serial.print("BT init " + btName + "...");
  
  if (serialBt_.begin(btName)) {
    Serial.println("ok");
  }
  else
  {
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
  if (int packetSize = LoRa.parsePacket()) {
    onLoraDataAvailable(packetSize);
  }
  // TX path, Serial -> Rig
  else {
    long currentTime = millis();
    if (currentTime > csmaSlotTimePrev_ + csmaSlotTime_ && random(0, 255) < csmaP_) {
      if (aprsisConn_.available() > 0) {
        onAprsisDataAvailable();
      }
      else if (needsBeacon()) {
        sendPeriodicBeacon();
      } 
      else {
        serialProcessRx();
      }
      csmaSlotTimePrev_ = currentTime;
    }
  }
  delay(CfgPollDelayMs);
}

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
  }

  if (config_.EnableIsToRf && aprsisData.length() > 0) {
    AX25::Payload payload(aprsisData);
    if (payload.IsValid()) {
      sendAX25ToLora(payload);
    }
    else {
      Serial.println("Unknown payload from APRSIS, ignoring...");
    }
  }
}

void Service::sendSignalReportEvent(int rssi, float snr)
{
  struct LoraSignalLevelEvent event;

  event.rssi = htobe16(rssi);
  event.snr = htobe16(snr * 100);

  serialSend(Cmd::RadioSignalLevel, (const byte *)&event, sizeof(LoraSignalLevelEvent));
}

bool Service::sendAX25ToLora(const AX25::Payload &payload) 
{
  byte buf[CfgMaxAX25PayloadSize];
  int bytesWritten = payload.ToBinary(buf, sizeof(buf));
  if (bytesWritten <= 0) {
    Serial.println("Failed to serialize payload");
    return false;
  }
  LoRa.beginPacket();
  LoRa.write(buf, bytesWritten);
  LoRa.endPacket();
  return true;
}

void Service::onLoraDataAvailable(int packetSize)
{
  int rxBufIndex = 0;
  byte rxBuf[packetSize];

  while (LoRa.available()) {
    byte rxByte = LoRa.read();
    rxBuf[rxBufIndex++] = rxByte;
    yield();
  }
  serialSend(Cmd::Data, rxBuf, rxBufIndex);
  long frequencyError = LoRa.packetFrequencyError();
  
  if (config_.EnableAutoFreqCorrection) {
    config_.LoraFreq -= frequencyError;
    LoRa.setFrequency(config_.LoraFreq);
  }

  if (config_.EnableKissExtensions) {
    sendSignalReportEvent(LoRa.packetRssi(), LoRa.packetSnr());
  }
  
  if (!config_.IsClientMode) {
    processIncomingRawPacketAsServer(rxBuf, rxBufIndex);
  }
}

void Service::processIncomingRawPacketAsServer(const byte *packet, int packetLength) {

  float snr = LoRa.packetSnr();
  int rssi = LoRa.packetRssi();
  long frequencyError = LoRa.packetFrequencyError();
  
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

  AX25::Payload payload(packet, packetLength);

  if (payload.IsValid()) {
    
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
  delay(CfgPollDelayMs);  // LoRa may drop packet if removed
  return (LoRa.beginPacket() == 1);
}

void Service::onRigTx(byte b)
{
  LoRa.write(b);
}

void Service::onRigTxEnd()
{
  LoRa.endPacket(true);
}

void Service::onSerialTx(byte b)
{
  serialBt_.write(b);
}

bool Service::onSerialRxHasData()
{
  return serialBt_.available();
}

bool Service::onSerialRx(byte *b)
{
  int rxResult = serialBt_.read();
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
      csmaP_ = value;
      break;
    case Cmd::SlotTime:
      csmaSlotTime_ = (long)value * 10;
      break;
    default:
      break;
  }
}

void Service::onRadioControlCommand(const std::vector<byte> &rawCommand) {

  if (config_.EnableKissExtensions && rawCommand.size() == sizeof(LoraControlCommand)) {
    const struct LoraControlCommand * controlCommand = reinterpret_cast<const struct LoraControlCommand*>(rawCommand.data());
    
    config_.LoraFreq = be32toh(controlCommand->freq);
    config_.LoraBw = be32toh(controlCommand->bw);
    config_.LoraSf = be16toh(controlCommand->sf);
    config_.LoraCodingRate = be16toh(controlCommand->cr);
    config_.LoraPower = be16toh(controlCommand->pwr);
    config_.LoraSync = be16toh(controlCommand->sync);
    config_.LoraEnableCrc = controlCommand->crc;

    setupLora(config_.LoraFreq, config_.LoraBw, config_.LoraSf, 
      config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, config_.LoraEnableCrc);
  }
}

} // LoraPrs
