#include "loraprs_service.h"

namespace LoraPrs {
  
Service::Service() 
  : csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , kissState_(KissState::Void)
  , kissCmd_(KissCmd::NoCmd)
  , kissTxQueue_(new cppQueue(sizeof(unsigned char), CfgLoraTxQueueSize))
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

void Service::reconnectWifi()
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
  Serial.print("LoRa init...");
  
  LoRa.setPins(CfgPinSs, CfgPinRst, CfgPinDio0);
  
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

  // RX path
  if (int packetSize = LoRa.parsePacket()) {
    onLoraDataAvailable(packetSize);
  }
  // TX path
  else {
    if (random(0, 255) < csmaP_) {
      if (aprsisConn_.available() > 0) {
        onAprsisDataAvailable();
      }
      else if (needsBeacon()) {
        sendPeriodicBeacon();
      } 
      else {
        processTx();
      }
    }
    else {
      delay(csmaSlotTime_);
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

bool Service::sendAX25ToLora(const AX25::Payload &payload) 
{
  byte buf[512];
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

  serialBt_.write(KissMarker::Fend);
  serialBt_.write(KissCmd::Data);

  while (LoRa.available()) {
    byte rxByte = LoRa.read();

    if (rxByte == KissMarker::Fend) {
      serialBt_.write(KissMarker::Fesc);
      serialBt_.write(KissMarker::Tfend);
    }
    else if (rxByte == KissMarker::Fesc) {
      serialBt_.write(KissMarker::Fesc);
      serialBt_.write(KissMarker::Tfesc);
    }
    else {
      rxBuf[rxBufIndex++] = rxByte;
      serialBt_.write(rxByte);
    }
  }

  serialBt_.write(KissMarker::Fend);

  float snr = LoRa.packetSnr();
  float rssi = LoRa.packetRssi();
  long frequencyError = LoRa.packetFrequencyError();

  if (config_.EnableAutoFreqCorrection) {
    config_.LoraFreq -= frequencyError;
    LoRa.setFrequency(config_.LoraFreq);
  }

  if (!config_.IsClientMode) {
    
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

    AX25::Payload payload(rxBuf, rxBufIndex);

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
    }
    else {
      Serial.println("Skipping non-AX25 payload");
    }
  }
}

void Service::processTx() 
{
  while (serialBt_.available() || !kissTxQueue_->isEmpty()) {

    if (serialBt_.available()) {
      int rxResult = serialBt_.read();
      if (rxResult != -1) {
        byte rxByte = (byte)rxResult;
        if (!kissTxQueue_->push((void *)&rxByte)) {
          Serial.println("TX queue is full");
        }
      }
    }
    if (!kissTxQueue_->isEmpty()) {
      byte qRxByte;
      if (kissTxQueue_->peek((void *)&qRxByte)) {
        if (kissReceiveByte(qRxByte)) {
          kissTxQueue_->drop();
        }
      }
    }
    yield();
  }
}

void Service::kissResetState()
{
  kissCmd_ = KissCmd::NoCmd;
  kissState_ = KissState::Void;
}

bool Service::kissProcessCommand(unsigned char rxByte) {

  switch (rxByte) {
    case KissCmd::Data:
      delay(CfgPollDelayMs);  // LoRa may drop packet if removed
      if (LoRa.beginPacket() == 0) return false;
      kissState_ = KissState::GetData;
      break;
    case KissCmd::P:
      kissState_ = KissState::GetP;
      break;
    case KissCmd::SlotTime:
      kissState_ = KissState::GetSlotTime;
      break;
    default:
      // unknown command
      kissResetState();
      return true;
  }
  kissCmd_ = (KissCmd)rxByte;
  return true;
}

bool Service::kissReceiveByte(unsigned char rxByte) {

  switch (kissState_) {
    case KissState::Void:
      if (rxByte == KissMarker::Fend) {
        kissCmd_ = KissCmd::NoCmd;
        kissState_ = KissState::GetCmd;
      }
      break;
    case KissState::GetCmd:
      if (rxByte != KissMarker::Fend) {
        if (!kissProcessCommand(rxByte)) return false;
      }
      break;
    case KissState::GetP:
      csmaP_ = rxByte;
      kissState_ = KissState::GetData;
      break;
    case KissState::GetSlotTime:
      csmaSlotTime_ = (long)rxByte * 10;
      kissState_ = KissState::GetData;
      break;
    case KissState::GetData:
      if (rxByte == KissMarker::Fesc) {
        kissState_ = KissState::Escape;
      }
      else if (rxByte == KissMarker::Fend) {
        if (kissCmd_ == KissCmd::Data) {
          LoRa.endPacket(true);
        }
        kissResetState();
      }
      else if (kissCmd_ == KissCmd::Data) {
        LoRa.write(rxByte);
      }
      break;
    case KissState::Escape:
      if (rxByte == KissMarker::Tfend) {
        LoRa.write(KissMarker::Fend);
        kissState_ = KissState::GetData;
      }
      else if (rxByte == KissMarker::Tfesc) {
        LoRa.write(KissMarker::Fesc);
        kissState_ = KissState::GetData;
      }
      else {
        kissResetState();
      }
      break;
    default:
      break;
  }
  return true;
}

} // LoraPrs
