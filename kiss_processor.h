#ifndef KISS_PROCESSOR_H
#define KISS_PROCESSOR_H

#include <Arduino.h>
#include <DebugLog.h>
#include <memory>

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>

namespace Kiss {

class Processor {
  
protected:
  enum Marker {
    Fend = 0xc0,
    Fesc = 0xdb,
    Tfend = 0xdc,
    Tfesc = 0xdd
  };

  enum State {
    GetStart = 0,
    GetEnd,
    GetCmd,
    GetData,
    GetP,
    GetSlotTime,
    Escape
  };

  enum Cmd {
    Data = 0x00,
    TxDelay = 0x01,
    P = 0x02,
    SlotTime = 0x03,
    TxTail = 0x04,
    SetHardware = 0x06,
    SignalReport = 0x07,
    RebootRequested = 0x08,
    NoCmd = 0x80
  };

  enum DataType {
      Raw = 0,
      Control,
      Reboot
  };

  static const int CfgSerialToRigQueueSize = 4096;
  static const int CfgRigToSerialQueueSize = 4096;

public:
  Processor();
  
  void sendRigToSerial(Cmd cmd, const byte *packet, int packetLength);
  static void ICACHE_RAM_ATTR queueRigToSerialIsr(Cmd cmd, const byte *packet, int packetLength);

  void queueSerialToRig(Cmd cmd, const byte *packet, int packetLength);

  bool processRigToSerial();
  bool processSerialToRig();

protected:
  virtual bool onRigTxBegin() = 0;
  virtual void onRigTx(byte b) = 0;
  virtual void onRigTxEnd() = 0;
  virtual void onRigPacket(void *packet, int packetLength) = 0;
  
  virtual void onSerialTx(byte b) = 0;
  virtual bool onSerialRxHasData() = 0;
  virtual bool onSerialRx(byte *b) = 0;

  virtual void onControlCommand(Cmd cmd, byte value) = 0;
  virtual void onRadioControlCommand(const std::vector<byte> &command) = 0;
  virtual void onRebootCommand() = 0;

private:
  bool receiveByte(byte rxByte);
  bool receiveByteRaw(byte rxByte);
  bool receiveByteKiss(byte rxByte);

  void processData(byte rxByte);
  bool processCommand(byte rxByte);

protected:
  bool disableKiss_;

private:
  bool isRawIdle_;
  State state_;
  DataType dataType_;
  std::vector<byte> cmdBuffer_;

  static CircularBuffer<uint8_t, CfgSerialToRigQueueSize> serialToRigQueue_;
  static CircularBuffer<uint8_t, CfgRigToSerialQueueSize> rigToSerialQueue_;
  static CircularBuffer<uint8_t, CfgRigToSerialQueueSize> rigToSerialQueueIndex_;
};
  
} // Kiss

#endif // KISS_PROCESSOR_H
