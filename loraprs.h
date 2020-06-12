#ifndef LORAPRS_H
#define LORAPRS_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

#include "BluetoothSerial.h"
#include "ax25_payload.h"

class LoraPrsConfig
{
public:
  bool IsClientMode;
  
  long LoraFreq;
  int LoraBw;
  byte LoraSf;
  byte LoraCodingRate;
  byte LoraSync;
  byte LoraPower;

  int AprsPort;
  String AprsHost;
  String AprsLogin;
  String AprsPass;
  String AprsFilter;
  
  String BtName;
  
  String WifiSsid;
  String WifiKey;

  bool EnableSignalReport;
  bool EnableAutoFreqCorrection;
  bool EnablePersistentAprsConnection;
  bool EnableIsToRf;
  bool EnableRepeater;
};

class LoraPrs
{
public:
  LoraPrs();
  
  void setup(const LoraPrsConfig &conf);
  void loop();

private:
  void setupWifi(const String &wifiName, const String &wifiKey);
  void setupLora(int loraFreq, int bw, byte sf, byte cr, byte pwr, byte sync);
  void setupBt(const String &btName);

  void reconnectWifi();
  bool reconnectAprsis();

  void onLoraDataAvailable(int packetSize);
  void onBtDataAvailable();

  void onRfAprsReceived(const String &aprsMessage);

  void kissResetState();

private:
  enum KissMarker {
    Fend = 0xc0,
    Fesc = 0xdb,
    Tfend = 0xdc,
    Tfesc = 0xdd
  };

  enum KissState {
    Void = 0,
    GetCmd,
    GetData,
    Escape
  };

  enum KissCmd {
    Data = 0x00,
    NoCmd = 0x80
  };

  const String CfgLoraprsVersion = "LoRAPRS 0.1";

  const byte CfgPinSs = 5;
  const byte CfgPinRst = 26;
  const byte CfgPinDio0 = 14;

private:
  bool isClient_;
  long loraFreq_;

  String aprsHost_;
  int aprsPort_;
  String aprsLogin_;

  bool autoCorrectFreq_;
  bool addSignalReport_;
  bool persistentConn_;
  bool enableIsToRf_;
  bool enableRepeater_;

  KissCmd kissCmd_;
  KissState kissState_;

  BluetoothSerial serialBt_;
  WiFiClient aprsisConn_;
};

#endif // LORAPRS_H
