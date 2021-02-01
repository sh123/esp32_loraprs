#ifndef LORAPRS_SEVICE_H
#define LORAPRS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <cppQueue.h>

#include "BluetoothSerial.h"
#include "ax25_payload.h"
#include "loraprs_config.h"

namespace LoraPrs {

class Service
{
public:
  Service();
  
  void setup(const Config &conf);
  void loop();

private:
  void setupWifi(const String &wifiName, const String &wifiKey);
  void setupLora(int loraFreq, int bw, byte sf, byte cr, byte pwr, byte sync, bool enableCrc);
  void setupBt(const String &btName);

  void reconnectWifi();
  bool reconnectAprsis();

  void processTx();
  
  void onLoraDataAvailable(int packetSize);
  void onAprsisDataAvailable();

  void sendPeriodicBeacon();
  void sendToAprsis(String aprsMessage);
  bool sendAX25ToLora(const AX25::Payload &payload);
  
  bool kissReceiveByte(unsigned char rxByte);
  bool kissProcessCommand(unsigned char rxByte);
  void kissResetState();

  inline bool needsAprsis() const { return !isClient_ && (enableRfToIs_ || enableIsToRf_); }
  inline bool needsWifi() const { return needsAprsis(); }
  inline bool needsBt() const { return isClient_; }
  inline bool needsBeacon() const { return !isClient_ && enableBeacon_; }

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
    GetP,
    GetSlotTime,
    Escape
  };

  enum KissCmd {
    Data = 0x00,
    P = 0x02,
    SlotTime = 0x03,
    NoCmd = 0x80
  };

  const String CfgLoraprsVersion = "LoRAPRS 0.1";

  const int CfgConnRetryMs = 500;
  const int CfgPollDelayMs = 5;
  const int CfgLoraTxQueueSize = 4096;
  const int CfgWiFiConnRetryMaxTimes = 10;

  // tx when lower than this value from random 0..255
  // use lower value for high traffic, use 255 for real time
  const long CfgCsmaPersistence = 100;
  const long CfgCsmaSlotTimeMs = 500;

  const byte CfgPinSs = 5;
  const byte CfgPinRst = 26;
  const byte CfgPinDio0 = 14;

private:
  // config
  bool isClient_;
  long loraFreq_;

  AX25::Callsign ownCallsign_;

  String aprsHost_;
  int aprsPort_;
  String aprsLogin_;
  String aprsBeacon_;
  int aprsBeaconPeriodMinutes_;

  bool autoCorrectFreq_;
  bool addSignalReport_;
  bool persistentConn_;
  bool enableRfToIs_;
  bool enableIsToRf_;
  bool enableRepeater_;
  bool enableBeacon_;

  // state
  KissState kissState_;
  KissCmd kissCmd_;
  long previousBeaconMs_;
  byte csmaP_;
  long csmaSlotTime_;
  std::shared_ptr<cppQueue>txQueue_;
  
  // peripherals
  BluetoothSerial serialBt_;
  WiFiClient aprsisConn_;
};

} // LoraPrs

#endif // LORAPRS_SERVICE_H
