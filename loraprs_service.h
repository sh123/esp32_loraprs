#ifndef LORAPRS_SEVICE_H
#define LORAPRS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <endian.h>

#include "BluetoothSerial.h"
#include "ax25_payload.h"
#include "kiss_processor.h"
#include "loraprs_config.h"

namespace LoraPrs {

class Service : public Kiss::Processor
{
public:
  Service();
  
  void setup(const Config &conf);
  void loop();

private:
  void setupWifi(const String &wifiName, const String &wifiKey);
  void setupLora(long loraFreq, long bw, int sf, int cr, int pwr, int sync, bool enableCrc);
  void setupBt(const String &btName);

  void reconnectWifi() const;
  bool reconnectAprsis();

  static ICACHE_RAM_ATTR void onLoraDataAvailableIsr(int packetSize);
  
  void loraReceive(int packetSize);
  void onAprsisDataAvailable();

  void sendSignalReportEvent(int rssi, float snr);
  void sendPeriodicBeacon();
  void sendToAprsis(const String &aprsMessage);
  bool sendAX25ToLora(const AX25::Payload &payload);
  void processIncomingRawPacketAsServer(const byte *packet, int packetLength);
  
  inline bool needsAprsis() const { 
    return !config_.IsClientMode && (config_.EnableRfToIs || config_.EnableIsToRf); 
  }
  inline bool needsWifi() const { return needsAprsis(); }
  inline bool needsBt() const { return config_.IsClientMode; }
  inline bool needsBeacon() const { return config_.EnableBeacon; }

protected:
  virtual bool onRigTxBegin();
  virtual void onRigTx(byte b);
  virtual void onRigTxEnd();
  virtual void onRigPacket(void *packet, int packetLength);
  
  virtual void onSerialTx(byte b);
  virtual bool onSerialRxHasData();
  virtual bool onSerialRx(byte *b);

  virtual void onControlCommand(Cmd cmd, byte value);
  virtual void onRadioControlCommand(const std::vector<byte> &command);

private:
  struct SetHardware {
    uint32_t freq;
    uint32_t bw;
    uint16_t sf;
    uint16_t cr;
    uint16_t pwr;
    uint16_t sync;
    uint8_t crc;
  } __attribute__((packed));

  struct SignalReport {
    int16_t rssi;
    int16_t snr;
  } __attribute__((packed));
  
private:
  const String CfgLoraprsVersion = "LoRAPRS 0.1";

  // processor config
  const int CfgConnRetryMs = 500;           // connection retry delay, e.g. wifi
  const int CfgPollDelayMs = 5;             // main loop delay
  const int CfgWiFiConnRetryMaxTimes = 10;  // wifi number of connection retries
  const int CfgMaxAX25PayloadSize = 512;    // maximum ax25 payload size
  const int CfgMaxAprsInMessageSize = 255;  // maximum aprsis to rf message size

  // csma parameters, overriden with KISS commands
  const long CfgCsmaPersistence = 100;  // 255 for real time, lower for higher traffic
  const long CfgCsmaSlotTimeMs = 500;   // 0 for real time, otherwise set to average tx duration
  
private:
  // config
  Config config_;
  String aprsLoginCommand_;
  AX25::Callsign ownCallsign_;

  // csma
  byte csmaP_;
  long csmaSlotTime_;
  long csmaSlotTimePrev_;

  // state
  long previousBeaconMs_;
    
  // peripherals
  BluetoothSerial serialBt_;
  WiFiClient aprsisConn_;
};

} // LoraPrs

#endif // LORAPRS_SERVICE_H
