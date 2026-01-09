#ifndef BLE_SERIAL_H
#define BLE_SERIAL_H

#include "Arduino.h"
#include "Stream.h"

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.hpp>

#define DEBUGLOG_DEFAULT_LOG_LEVEL_INFO
#include <DebugLog.h>

#include "loraprs_config.h"

namespace LoraPrs {

class BLESerial: public Stream
{
  friend class BLESerialServerCallbacks;
  friend class BLESerialCharacteristicCallbacks;

public:
  static constexpr const char* CfgServiceUuid = "00000001-ba2a-46c9-ae49-01b0961f68bb"; // KISS service UUID
  static constexpr const char* CfgCharacteristicUuidTx = "00000003-ba2a-46c9-ae49-01b0961f68bb";
  static constexpr const char* CfgCharacteristicUuidRx = "00000002-ba2a-46c9-ae49-01b0961f68bb";

  static constexpr int CfgQueueSize = 256;
  static constexpr int CfgHdrMtuSize = 3;
  static constexpr int CfgMinMtuSize = 23;
  static constexpr int CfgMaxMtuSize = 247;

public:
  BLESerial(void);
  ~BLESerial(void);

  bool begin(const Config &conf);
  int available(void) override;
  int peek(void) override;
  bool connected(void);
  int read(void) override;
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  void flush() override;
  void end(void);

private:
  size_t getMaxPayloadSize();
  void transmit();

private:
  Config config_;

  bool isConnected_;
  NimBLEServer *pServer_ = nullptr;
  NimBLEService *pService_ = nullptr;
  NimBLECharacteristic *pTxCharacteristic_ = nullptr;

  CircularBuffer<uint8_t, CfgQueueSize> transmitQueue_;
  CircularBuffer<uint8_t, CfgQueueSize> receiveQueue_;
};

} // LoraPrs

#endif // BLE_SERIAL_H
