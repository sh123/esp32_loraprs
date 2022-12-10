#ifndef LORAPRS_SEVICE_H
#define LORAPRS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>
#include <DebugLog.h>

// some generic options (module name, library type) are loaded from config.h
#if __has_include("/tmp/esp32_loraprs_config.h")
#include "/tmp/esp32_loraprs_config.h"
#else
#include "config.h"
#endif

#include <RadioLib.h>

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
  void setupRig(long freq, long bw, int sf, int cr, int pwr, int sync, int crcBytes, bool isExplicit);
  void setFreq(long freq) const;
  void setupBt(const String &btName);

  void reconnectWifi() const;
  bool reconnectAprsis();
  void attachKissNetworkClient();

  bool isRigRxBusy();
  void onRigTaskRxPacket();
  void onRigTaskTxPacket();
  static void rigTask(void *self);
  static ICACHE_RAM_ATTR void onRigIsrRxPacket();

  void onAprsisDataAvailable();

  void sendSignalReportEvent(int rssi, float snr);
  void sendPeriodicBeacon();
  void sendToAprsis(const String &aprsMessage);
  bool sendAx25PayloadToRig(const AX25::Payload &payload);
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
  inline bool splitEnabled() const {
    return config_.LoraFreqRx != config_.LoraFreqTx;
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
  const String CfgLoraprsVersion = "LoRAPRS 1.0.3";

  // processor config
  const int CfgConnRetryMs = 500;             // connection retry delay, e.g. wifi
  const int CfgPollDelayMs = 20;              // main loop delay
  const int CfgConnRetryMaxTimes = 10;        // number of connection retries
  static const int CfgMaxPacketSize = 256;    // maximum packet size
  static const int CfgRadioQueueSize = 1024;  // radio queue size

  // csma parameters, overriden with KISS commands
  const long CfgCsmaPersistence = 100;  // 255 for real time, lower for higher traffic
  const long CfgCsmaSlotTimeMs = 500;   // 0 for real time, otherwise set to average tx duration

  // kiss static parameters
  const int CfgKissPort = 8001;             // kiss tcp/ip server port

  // radio task commands
  enum RadioTaskBits {
    Receive = 0x01,
    Transmit = 0x02
  };

private:
  // config
  Config config_;
  String aprsLoginCommand_;
  AX25::Callsign aprsMyCallsign_;

  // csma
  byte csmaP_;
  long csmaSlotTime_;
  long csmaSlotTimePrev_;

  // beacon state
  long beaconLastTimestampMs_;

  // peripherals, radio
  static TaskHandle_t rigTaskHandle_;
  static volatile bool rigIsRxActive_;
  static volatile bool rigIsRxIsrEnabled_;
  bool rigIsImplicitMode_;
  int rigCurrentTxPacketSize_;
  bool isIsrInstalled_;
  CircularBuffer<uint8_t, CfgRadioQueueSize> rigTxQueue_;
  CircularBuffer<uint8_t, CfgRadioQueueSize> rigTxQueueIndex_;
  std::shared_ptr<MODULE_NAME> rig_;

  // bluetooth, wifi
  BluetoothSerial serialBt_;
  BLESerial serialBLE_;
  WiFiClient aprsisConnection_;
  
  // kiss server
  std::shared_ptr<WiFiServer> kissServer_;
  WiFiClient kissConnnection_;
  bool isKissClientConnected_;
};

} // LoraPrs

#endif // LORAPRS_SERVICE_H
