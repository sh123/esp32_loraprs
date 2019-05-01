#include "loraprs.h"

LoraPrs::LoraPrs(int loraFreq, String btName, String wifiName, 
  String wifiKey, String aprsLoginCallsign, String aprsPass) 
  : serialBt_()
  , loraFreq_(loraFreq)
  , btName_(btName)
  , wifiName_(wifiName)
  , wifiKey_(wifiKey)
{ 
    aprsLogin_ = "";
    aprsLogin_ += "user ";
    aprsLogin_ += aprsLoginCallsign;
    aprsLogin_ += " pass ";
    aprsLogin_ += aprsPass;
    aprsLogin_ += " vers ";
    aprsLogin_ += CfgLoraprsVersion;
    aprsLogin_ += "\n";
}

void LoraPrs::setup()
{
  setupWifi(wifiName_, wifiKey_);
  setupLora(loraFreq_);
  setupBt(btName_);
}

void LoraPrs::setupWifi(String wifiName, String wifiKey) 
{
  if (wifiName.length() != 0) {    
    Serial.print("WIFI connecting to " + wifiName);

    WiFi.begin(wifiName.c_str(), wifiKey.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("ok");
    Serial.println(WiFi.localIP());
  }
}

void LoraPrs::setupLora(int loraFreq)
{
  Serial.print("LoRa init...");
  
  LoRa.setPins(CfgPinSs, CfgPinRst, CfgPinDio0);
  
  while (!LoRa.begin(loraFreq)) {
    Serial.print(".");
    delay(500);
  }
  LoRa.setSyncWord(CfgSync);
  LoRa.setSpreadingFactor(CfgSpread);
  LoRa.setSignalBandwidth(CfgBw);
  LoRa.setCodingRate4(CfgCodingRate);
  LoRa.setTxPower(CfgPower);
  
  Serial.println("ok");  
}

void LoraPrs::setupBt(String btName) 
{
  if (btName.length() != 0) {
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
  if (serialBt_.available()) {
    onBtReceived();
  }
  if (LoRa.parsePacket()) {
    onLoraReceived();
  }
}

void LoraPrs::onAprsReceived(String aprsMessage)
{
  Serial.print(aprsMessage);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient wifiClient;

    if (!wifiClient.connect(CfgAprsHost.c_str(), CfgAprsPort)) {
      Serial.println("Failed to connect to " + CfgAprsHost + ":" + CfgAprsPort);
      return;
    }
    wifiClient.print(aprsLogin_);
    wifiClient.print(aprsMessage);
    wifiClient.stop();
  }
}

void LoraPrs::onLoraReceived()
{
  String buf;
  while (LoRa.available()) {
    char c = (char)LoRa.read();
    if (c != '\n')
      buf += c;
  }
  for (int i; i < buf.length(); i++) {
    serialBt_.write((uint8_t)buf[i]);
  }
  onAprsReceived(buf + " " +
    String(LoRa.packetRssi()) + ", " +
    String(LoRa.packetSnr()) + "dB, " +
    String(LoRa.packetFrequencyError()) + "ppm\n");
  delay(50);
}

void LoraPrs::onBtReceived() 
{
  LoRa.beginPacket();
  while (serialBt_.available()) {
    char c = (char)serialBt_.read();
    LoRa.print(c);
  }
  LoRa.endPacket();
}
