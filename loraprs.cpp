#include "loraprs.h"

LoraPrs::LoraPrs(int loraFreq, const String & btName, const String & wifiName, 
  const String & wifiKey, const String & aprsLoginCallsign, const String & aprsPass) 
  : serialBt_()
  , loraFreq_(loraFreq)
  , btName_(btName)
  , wifiName_(wifiName)
  , wifiKey_(wifiKey)
  , kissState_(KissState::Void)
  , kissCmd_(KissCmd::NoCmd)
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

void LoraPrs::setupWifi(const String & wifiName, const String & wifiKey) 
{
  if (wifiName.length() != 0) {    
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

void LoraPrs::reconnectWifi() {

  Serial.print("WIFI re-connecting...");

  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(500);
    Serial.print(".");
  }

  Serial.println("ok");
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
  LoRa.enableCrc();
  
  Serial.println("ok");  
}

void LoraPrs::setupBt(const String & btName)
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
  if (WiFi.status() != WL_CONNECTED && wifiName_.length() != 0) {
    reconnectWifi();
  }
  if (serialBt_.available()) {
    onBtReceived();
  }
  if (int packetSize = LoRa.parsePacket()) {
    onLoraReceived(packetSize);
  }
  delay(10);
}

void LoraPrs::onAprsReceived(const String & aprsMessage)
{
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient wifiClient;

    if (!wifiClient.connect(CfgAprsHost.c_str(), CfgAprsPort)) {
      Serial.println("Failed to connect to " + CfgAprsHost + ":" + CfgAprsPort);
      return;
    }
    wifiClient.print(aprsLogin_);
    Serial.print(aprsMessage);
    wifiClient.print(aprsMessage);
    wifiClient.stop();
  } 
  else {
    Serial.println("Wifi not connected, not sent");
  }
}

String LoraPrs::decodeCall(byte *rxPtr) 
{
  byte callsign[7];
  char ssid;
  
  byte *ptr = rxPtr;

  memset(callsign, 0, sizeof(callsign));
    
  for (int i = 0; i < 6; i++) {
    char c = *(ptr++) >> 1;
    callsign[i] = (c == ' ') ? '\0' : c;
  }
  callsign[6] = '\0';
  ssid = (*ptr >> 1);
  
  String result = String((char*)callsign);
  if (ssid >= '0' && ssid <= '9') {
    result += String("-") + String(ssid);
  }
  return result;
}

String LoraPrs::convertAX25ToAprs(byte *rxPayload, int payloadLength, const String & signalReport)
{
  byte *rxPtr = rxPayload;
  String srcCall, dstCall, rptFirst, rptSecond, result;

  dstCall = decodeCall(rxPtr);
  rxPtr += 7;

  srcCall = decodeCall(rxPtr);
  rxPtr += 7;
  
  if ((rxPayload[13] & 1) == 0) {
    rptFirst = decodeCall(rxPtr);
    rxPtr += 7;

    if ((rxPayload[20] & 1) == 0) {
      rptSecond = decodeCall(rxPtr);
      rxPtr += 7;
    }
  }
  
  if (*(rxPtr++) != AX25Ctrl::UI) return result;
  if (*(rxPtr++) != AX25Pid::NoLayer3) return result;
  
  result += srcCall + String(">") + dstCall;
    
  if (rptFirst.length() > 0) {
    result += String(",") + rptFirst;
  }
  if (rptSecond.length() > 0) {
    result += String(",") + rptSecond;
  }

  result += ":";

  bool appendReport = ((char)*rxPtr == '=');
  
  while (rxPtr < rxPayload + payloadLength) {
    result += String((char)*(rxPtr++));
  }

  if (appendReport) {
    result += signalReport;
  }
  else {
    result += "\n";
  }
  
  return result;
}

void LoraPrs::onLoraReceived(int packetSize)
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

  String signalReport = String(" ") +  
    String("RSSI: ") + 
    String(LoRa.packetRssi()) + 
    String(", ") +
    String("SNR: ") + 
    String(LoRa.packetSnr()) + 
    String("dB, ") +
    String("ERR: ") + 
    String(LoRa.packetFrequencyError()) + 
    String("Hz\n");

  String aprsMsg = convertAX25ToAprs(rxBuf, rxBufIndex, signalReport);

  if (aprsMsg.length() != 0) {
    onAprsReceived(aprsMsg);
  }

  delay(50);
}

void LoraPrs::kissResetState()
{
  kissCmd_ = KissCmd::NoCmd;
  kissState_ = KissState::Void;
}

void LoraPrs::onBtReceived() 
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
