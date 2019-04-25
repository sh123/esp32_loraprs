#ifndef LORAPRS_H
#define LORAPRS_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

#include "BluetoothSerial.h"

class LoraPrs
{
public:
  const String CfgLoraprsVersion = "LoRAPRS 0.1";
  
  const byte CfgPinSs = 5;
  const byte CfgPinRst = 26;
  const byte CfgPinDio0 = 14;

  const int CfgBw = 20e3;
  const byte CfgSpread = 9;
  const byte CfgCodingRate = 5;
  const byte CfgSync = 0xf3;
  const byte CfgPower = 20;

  const int CfgAprsPort = 14580;
  const String CfgAprsHost = "rotate.aprs2.net";

public:
  LoraPrs(int freq, String btName, String wifiName, 
    String wifiKey, String aprsLoginCallsign, String aprsPass);
  
  void setup();
  void loop();

private:
  void setupWifi(String wifiName, String wifiKey);
  void setupLora(int loraFreq);
  void setupBt(String btName);
  
  void onLoraReceived();
  void onBtReceived();
  void onAprsReceived(String aprsMessage);
  
private:
  BluetoothSerial serialBt_;
  
  int loraFreq_;
  String btName_;
  String wifiName_;
  String wifiKey_;
  String aprsLogin_;
};

#endif // LORAPRS_H
