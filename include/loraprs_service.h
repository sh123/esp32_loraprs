#ifndef LORAPRS_SERVICE_H
#define LORAPRS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>

#define DEBUGLOG_DEFAULT_LOG_LEVEL_INFO
#include <DebugLog.h>

#include "config.h"
#include "version.h"

#include <RadioLib.h>

#include <WiFi.h>
#include <endian.h>
#include <arduino-timer.h>

#include "BluetoothSerial.h"
#include "ble_serial.h"
#include "ax25_payload.h"
#include "kiss_processor.h"
#include "loraprs_config.h"

namespace LoraPrs {

class Service : virtual public Kiss::Processor
{
public:
  Service();
  
  void setup(const Config &conf);
  void loop();

private:
  void printConfig() const;

  void setupWifi(const String &wifiName, const String &wifiKey);
  void setupRig(long freq, long bw, int sf, int cr, int pwr, int sync, int crcBytes, bool isExplicit);
  void setupRigFsk(long freq, float bitRate, float freqDev, float rxBw, int pwr);
  void setupBt(const String &btName);

  void setFreq(long freq) const;

  void reconnectWifi() const;
  bool reconnectAprsis();
  void attachKissNetworkClient();

  inline bool isRigRxBusy() const { return config_.LoraUseCad && rigIsRxActive_; }

  void onRigTaskRxPacket();
  void onRigTaskTxPacket();
  void onRigTaskStartRx();
  void onRigTaskStartTx();
  static void rigTask(void *self);
  static ICACHE_RAM_ATTR void onRigIsrRxPacket();

  void startRx();
  static bool startRxTimer(void *param);

  void onAprsisDataAvailable();

  void sendSignalReportEvent(int rssi, float snr);
  static bool sendModemTelemetryTimer(void *param);
  void sendModemTelemetry();
  void sendPeriodicBeacon();
  void sendToAprsis(const String &aprsMessage);
  bool sendAx25PayloadToRig(const AX25::Payload &payload);
  void processIncomingRawPacketAsServer(const byte *packet, int packetLength);
  void performFrequencyCorrection();

  inline bool needsAprsis() const { 
    return !config_.IsClientMode  // only in server mode
      && (config_.EnableRfToIs || config_.EnableIsToRf)  // AND rx/tx igate enabled
      && !config_.WifiEnableAp;  // AND wifi is NOT in AP mode otherwise no internet
  }
  inline bool needsWifi() const { 
    return needsAprsis()  // aprsis is needed
      || config_.KissEnableTcpIp; // OR kiss over tcp ip is enabled
  }
  inline bool needsBt() const { 
    return (config_.IsClientMode || config_.BtName.length() > 0)  // client mode or name must be specified
      && !config_.UsbSerialEnable;  // AND no usb serial mode
  }
  inline bool needsBeacon() const { 
    return !config_.IsClientMode  // beaconing only in apris gate / server mode
      && config_.EnableBeacon;  // AND beacon must be explicitly enabled
  }
  inline bool isHalfDuplex() const {
    return config_.LoraFreqRx != config_.LoraFreqTx;
  }

  inline int getSpeed(int sf, int cr, long bw) const { return (int)(sf * (4.0 / cr) / (pow(2.0, sf) / bw)); }
  float getSnrLimit(int sf, long bw) const;

protected:
  virtual bool onRigTxBegin() override;
  virtual void onRigTx(byte b) override;
  virtual void onRigTxEnd() override;
  virtual void onRigPacket(void *packet, int packetLength) override;
  
  virtual void onSerialTx(byte b) override;
  virtual void onSerialTxEnd() override;
  virtual bool onSerialRxHasData() override;
  virtual bool onSerialRx(byte *b) override;

  virtual void onControlCommand(Cmd cmd, byte value) override;
  virtual void onRadioControlCommand(const std::vector<byte> &command) override;
  virtual void onRebootCommand() override;

private:
  struct SetHardware {
    uint32_t freqRx;
    uint32_t freqTx;
    uint8_t modType;
    int16_t pwr;
    uint32_t bw;
    uint16_t sf;
    uint16_t cr;
    uint16_t sync;
    uint8_t crc;
    uint32_t fskBitRate;
    uint32_t fskFreqDev;
    uint32_t fskRxBw;
  } __attribute__((packed));

  struct SignalReport {
    int16_t rssi;
    int16_t snr;
  } __attribute__((packed));

  struct Telemetry {
    int16_t batteryVoltage;
  } __attribute__((packed));

private:
  const String CfgLoraprsVersion = String("LoRAPRS ") + LORAPRS_VERSION;

  // processor config
  const int CfgConnRetryMs = 500;             // connection retry delay, e.g. wifi
  const int CfgConnRetryMaxTimes = 10;        // number of connection retries
  const int CfgTelemetryPeriodMs = 60000;     // how often to send telemetry event

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
    Transmit = 0x02,
    StartReceive = 0x04,
    StartTransmit = 0x10
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
  Timer<1> startRxTimer_;
  static volatile bool rigIsRxActive_;
  static volatile bool rigIsRxIsrEnabled_;
  bool rigIsImplicitMode_;
  int rigCurrentTxPacketSize_;
  bool isIsrInstalled_;
  CircularBuffer<uint8_t, CfgRadioQueueSize> rigTxQueue_;
  CircularBuffer<uint8_t, CfgRadioQueueSize> rigTxQueueIndex_;
  std::shared_ptr<MODULE_NAME> rig_;

  // bluetooth, wifi
#if CFG_BT_USE_BLE == true
  BLESerial serialBLE_;
#else
  BluetoothSerial serialBt_;
#endif
  WiFiClient aprsisConnection_;
  
  // kiss server
  std::shared_ptr<WiFiServer> kissServer_;
  WiFiClient kissConnnection_;
  bool isKissClientConnected_;

  // modem telemetry
  Timer<1> telemetryTimer_;
};

} // LoraPrs

#endif // LORAPRS_SERVICE_H
