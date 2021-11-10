#ifndef LORAPRS_SEVICE_H
#define LORAPRS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>
#include <DebugLog.h>

// When USE_RADIOLIB is defined then RadioLib will be used, otherwise arduino-LoRa will be used
// When using RadioLib, default module is SX1278, if you are using
// different module then update MODULE_NAME in module_name.h
#define USE_RADIOLIB
#define HW_DJAPRS 2
#ifdef USE_RADIOLIB
#include <RadioLib.h>
#include "module_name.h"
#else
#include <LoRa.h>
#endif

#include <WiFi.h>
#include <endian.h>

#include "BluetoothSerial.h"
#include "ble_serial.h"
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
  void printConfig();

  void setupWifi(const String &wifiName, const String &wifiKey);
  void setupLora(long loraFreq, long bw, int sf, int cr, int pwr, int sync, bool enableCrc);
  void setupBt(const String &btName);

  void reconnectWifi() const;
  bool reconnectAprsis();
  void attachKissNetworkClient();

  bool isLoraRxBusy();
#ifdef USE_RADIOLIB
  void onLoraDataAvailable();
  static ICACHE_RAM_ATTR void onLoraDataAvailableIsr();
#else
  static ICACHE_RAM_ATTR void onLoraDataAvailableIsr(int packetSize);
  void loraReceive(int packetSize);
#endif
  void onAprsisDataAvailable();

  void sendSignalReportEvent(int rssi, float snr);
  void sendPeriodicBeacon();
  void sendToAprsis(const String &aprsMessage);
  bool sendAX25ToLora(const AX25::Payload &payload);
  void processIncomingRawPacketAsServer(const byte *packet, int packetLength);
  void performFrequencyCorrection();

  inline bool needsAprsis() const { 
    return !config_.IsClientMode  // only in server mode
      && (config_.EnableRfToIs || config_.EnableIsToRf)  // rx/tx igate enabled
      && !config_.WifiEnableAp;  // wifi is NOT in AP mode
  }
  inline bool needsWifi() const { 
    return needsAprsis()  // aprsis is needed
      || config_.KissEnableTcpIp; // or kiss over tcp ip is enabled
  }
  inline bool needsBt() const { 
    return (config_.IsClientMode || config_.BtName.length() > 0)  // client mode or name must be specified
      && !config_.UsbSerialEnable;  // inactive in usb serial mode
  }
  inline bool needsBeacon() const { 
    return !config_.IsClientMode  // beaconing only in apris gate / server mode
    && config_.EnableBeacon;  // beacon must be explicitly enabled
  }

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
  virtual void onRebootCommand();

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
  const int CfgConnRetryMaxTimes = 10;      // number of connection retries
  static const int CfgMaxPacketSize = 256;  // maximum packet size

  // csma parameters, overriden with KISS commands
  const long CfgCsmaPersistence = 100;  // 255 for real time, lower for higher traffic
  const long CfgCsmaSlotTimeMs = 500;   // 0 for real time, otherwise set to average tx duration

  // kiss static parameters
  const int CfgKissPort = 8001;             // kiss tcp/ip server port
private:
  // config
  Config config_;
  String aprsLoginCommand_;
  AX25::Callsign ownCallsign_;
  bool isImplicitHeaderMode_;

  // csma
  byte csmaP_;
  long csmaSlotTime_;
  long csmaSlotTimePrev_;

  // state
  long previousBeaconMs_;

  // peripherals
  static byte rxBuf_[CfgMaxPacketSize];
#ifdef USE_RADIOLIB
  static bool interruptEnabled_;
  CircularBuffer<uint8_t, CfgMaxPacketSize> txQueue_;
  static std::shared_ptr<MODULE_NAME> radio_;
#endif
  BluetoothSerial serialBt_;
  BLESerial serialBLE_;
  WiFiClient aprsisConn_;
  
  std::shared_ptr<WiFiServer> kissServer_;
  WiFiClient kissConn_;
  bool isKissConn_;
};

} // LoraPrs

#endif // LORAPRS_SERVICE_H
