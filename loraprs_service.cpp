#include "loraprs_service.h"

namespace LoraPrs {
  
Service::Service() 
  : kissState_(KissState::Void)
  , kissCmd_(KissCmd::NoCmd)
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , txQueue_(new cppQueue(sizeof(unsigned char), CfgLoraTxQueueSize))
  , serialBt_()
{
}

Service::~Service() {
  delete txQueue_;
}

void Service::setup(const Config &conf)
{
  previousBeaconMs_ = 0;

  // config
  isClient_ = conf.IsClientMode;
  loraFreq_ = conf.LoraFreq;
  ownCallsign_ = AX25::Callsign(conf.AprsLogin);
  if (!ownCallsign_.IsValid()) {
    Serial.println("Own callsign is not valid");
  }
  
  aprsLogin_ = String("user ") + conf.AprsLogin + String(" pass ") + 
    conf.AprsPass + String(" vers ") + CfgLoraprsVersion;
  if (conf.AprsFilter.length() > 0) {
    aprsLogin_ += String(" filter ") + conf.AprsFilter;
  }
  aprsLogin_ += String("\n");
  
  aprsHost_ = conf.AprsHost;
  aprsPort_ = conf.AprsPort;
  aprsBeacon_ = conf.AprsRawBeacon;
  aprsBeaconPeriodMinutes_ = conf.AprsRawBeaconPeriodMinutes;
  
  autoCorrectFreq_ = conf.EnableAutoFreqCorrection;
  addSignalReport_ = conf.EnableSignalReport;
  persistentConn_ = conf.EnablePersistentAprsConnection;
  enableRfToIs_ = conf.EnableRfToIs;
  enableIsToRf_ = conf.EnableIsToRf;
  enableRepeater_ = conf.EnableRepeater;
  enableBeacon_ = conf.EnableBeacon;

  // peripherals
  setupLora(conf.LoraFreq, conf.LoraBw, conf.LoraSf, 
    conf.LoraCodingRate, conf.LoraPower, conf.LoraSync, conf.LoraEnableCrc);
    
  if (needsWifi()) {
    setupWifi(conf.WifiSsid, conf.WifiKey);
  }

  if (needsBt() || conf.BtName.length() > 0) {
    setupBt(conf.BtName);
  }
  
  if (needsAprsis() && persistentConn_) {
    reconnectAprsis();
  }
}

void Service::setupWifi(const String &wifiName, const String &wifiKey)
{
  if (!isClient_) {
    Serial.print("WIFI connecting to " + wifiName);

    WiFi.setHostname("loraprs");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiName.c_str(), wifiKey.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("ok");
    Serial.println(WiFi.localIP());
  }
}

void Service::reconnectWifi()
{
  Serial.print("WIFI re-connecting...");

  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(500);
    Serial.print(".");
  }

  Serial.println("ok");
}

bool Service::reconnectAprsis()
{
  Serial.print("APRSIS connecting...");
  
  if (!aprsisConn_.connect(aprsHost_.c_str(), aprsPort_)) {
    Serial.println("Failed to connect to " + aprsHost_ + ":" + aprsPort_);
    return false;
  }
  Serial.println("ok");

  aprsisConn_.print(aprsLogin_);
  return true;
}

void Service::setupLora(int loraFreq, int bw, byte sf, byte cr, byte pwr, byte sync, bool enableCrc)
{
  Serial.print("LoRa init...");
  
  LoRa.setPins(CfgPinSs, CfgPinRst, CfgPinDio0);
  
  while (!LoRa.begin(loraFreq)) {
    Serial.print(".");
    delay(500);
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
  if (needsAprsis() && !aprsisConn_.connected() && persistentConn_) {
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
  
  if (previousBeaconMs_ == 0 || currentMs - previousBeaconMs_ >= aprsBeaconPeriodMinutes_ * 60 * 1000) {
      AX25::Payload payload(aprsBeacon_);
      if (payload.IsValid()) {
        sendAX25ToLora(payload);
        if (enableRfToIs_) {
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

void Service::sendToAprsis(String aprsMessage)
{
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConn_.connected()) {
    reconnectAprsis();
  }
  aprsisConn_.println(aprsMessage);

  if (!persistentConn_) {
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

  if (enableIsToRf_ && aprsisData.length() > 0) {
    AX25::Payload payload(aprsisData);
    if (payload.IsValid()) {
      sendAX25ToLora(payload);
    }
    else {
      Serial.println("Invalid payload from APRSIS");
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

  if (autoCorrectFreq_) {
    loraFreq_ -= frequencyError;
    LoRa.setFrequency(loraFreq_);
  }

  if (!isClient_) {
    
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
      String textPayload = payload.ToString(addSignalReport_ ? signalReport : String());
      Serial.println(textPayload);

      if (enableRfToIs_) {
        sendToAprsis(textPayload);
        Serial.println("Packet sent to APRS-IS");
      }
      if (enableRepeater_ && payload.Digirepeat(ownCallsign_)) {
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
  while (serialBt_.available() || !txQueue_->isEmpty()) {

    if (serialBt_.available()) {
      int rxResult = serialBt_.read();
      if (rxResult != -1) {
        byte rxByte = (byte)rxResult;
        if (!txQueue_->push((void *)&rxByte)) {
          Serial.println("TX queue is full");
        }
      }
    }
    if (!txQueue_->isEmpty()) {
      byte qRxByte;
      if (txQueue_->peek((void *)&qRxByte)) {
        if (kissReceiveByte(qRxByte)) {
          txQueue_->drop();
        }
      }
    }
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
