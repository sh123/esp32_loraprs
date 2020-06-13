#include "loraprs.h"

LoraPrs::LoraPrs() 
  : serialBt_()
  , kissState_(KissState::Void)
  , kissCmd_(KissCmd::NoCmd)
{ 
}

void LoraPrs::setup(const LoraPrsConfig &conf)
{
  isClient_ = conf.IsClientMode;
  loraFreq_ = conf.LoraFreq;
  
  aprsLogin_ = String("user ") + conf.AprsLogin + String(" pass ") + 
    conf.AprsPass + String(" vers ") + CfgLoraprsVersion;
  if (conf.AprsFilter.length() > 0) {
    aprsLogin_ += String(" filter ") + conf.AprsFilter;
  }
  aprsLogin_ += String("\n");
  
  aprsHost_ = conf.AprsHost;
  aprsPort_ = conf.AprsPort;
  
  autoCorrectFreq_ = conf.EnableAutoFreqCorrection;
  addSignalReport_ = conf.EnableSignalReport;
  persistentConn_ = conf.EnablePersistentAprsConnection;
  enableIsToRf_ = conf.EnableIsToRf;
  enableRepeater_ = conf.EnableRepeater;
  
  setupWifi(conf.WifiSsid, conf.WifiKey);
  
  setupLora(conf.LoraFreq, conf.LoraBw, conf.LoraSf, conf.LoraCodingRate, conf.LoraPower, conf.LoraSync);
  setupBt(conf.BtName);
  
  if (!isClient_ && persistentConn_) {
    reconnectAprsis();
  }
}

void LoraPrs::setupWifi(const String &wifiName, const String &wifiKey)
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

void LoraPrs::reconnectWifi()
{
  Serial.print("WIFI re-connecting...");

  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(500);
    Serial.print(".");
  }

  Serial.println("ok");
}

bool LoraPrs::reconnectAprsis()
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

void LoraPrs::setupLora(int loraFreq, int bw, byte sf, byte cr, byte pwr, byte sync)
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
  LoRa.enableCrc();
  
  Serial.println("ok");  
}

void LoraPrs::setupBt(const String &btName)
{
  if (isClient_) {
    Serial.print("BT init " + btName + "...");
  
    if (serialBt_.begin(btName)) {
      Serial.println("ok");
    }
    else
    {
      Serial.println("failed");
    }
  }
}

void LoraPrs::loop()
{
  if (!isClient_) {
    if (WiFi.status() != WL_CONNECTED) {
      reconnectWifi();
    }
    if (!aprsisConn_.connected()) {
      reconnectAprsis();
    }
    if (aprsisConn_.available() > 0) {
      onAprsisDataAvailable();
    }
  }
  if (serialBt_.available()) {
    onBtDataAvailable();
  }
  if (int packetSize = LoRa.parsePacket()) {
    onLoraDataAvailable(packetSize);
  }
  delay(10);
}

void LoraPrs::onRfAprsReceived(String aprsMessage)
{
  if (isClient_) return;
  
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (!aprsisConn_.connected()) {
    reconnectAprsis();
  }
  Serial.print(aprsMessage);
  aprsisConn_.print(aprsMessage);

  if (!persistentConn_) {
    aprsisConn_.stop();
  }
}

void LoraPrs::onAprsisDataAvailable()
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

      byte buf[512];
      int bytesWritten = payload.ToBinary(buf, sizeof(buf));
      if (bytesWritten > 0) {
        LoRa.beginPacket();
        LoRa.write(buf, bytesWritten);
        LoRa.endPacket(true);
      }
      else {
        Serial.println("Failed to serialize payload");
      }
    }
    else {
      Serial.println("Invalid payload from APRSIS");
    }
  }
}

void LoraPrs::onLoraDataAvailable(int packetSize)
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

  if (autoCorrectFreq_) {
    loraFreq_ -= frequencyError;
    LoRa.setFrequency(loraFreq_);
  }

  AX25::Payload payload(rxBuf, rxBufIndex);

  if (payload.IsValid()) {
    onRfAprsReceived(payload.ToText(addSignalReport_ ? signalReport : String()));
  }
  else {
    Serial.println("Invalid payload from LoRA");
  }

  delay(50);
}

void LoraPrs::kissResetState()
{
  kissCmd_ = KissCmd::NoCmd;
  kissState_ = KissState::Void;
}

void LoraPrs::onBtDataAvailable() 
{ 
  while (serialBt_.available()) {
    byte txByte = serialBt_.read();

    switch (kissState_) {
      case KissState::Void:
        if (txByte == KissMarker::Fend) {
          kissCmd_ = KissCmd::NoCmd;
          kissState_ = KissState::GetCmd;
        }
        break;
      case KissState::GetCmd:
        if (txByte != KissMarker::Fend) {
          if (txByte == KissCmd::Data) {
            LoRa.beginPacket();
            kissCmd_ = (KissCmd)txByte;
            kissState_ = KissState::GetData;
          }
          else {
            kissResetState();
          }
        }
        break;
      case KissState::GetData:
        if (txByte == KissMarker::Fesc) {
          kissState_ = KissState::Escape;
        }
        else if (txByte == KissMarker::Fend) {
          if (kissCmd_ == KissCmd::Data) {
            LoRa.endPacket();
          }
          kissResetState();
        }
        else {
          LoRa.write(txByte);
        }
        break;
      case KissState::Escape:
        if (txByte == KissMarker::Tfend) {
          LoRa.write(KissMarker::Fend);
          kissState_ = KissState::GetData;
        }
        else if (txByte == KissMarker::Tfesc) {
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
  }
  delay(20);
}
