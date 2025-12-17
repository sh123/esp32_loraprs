#ifndef KISS_PROCESSOR_H
#define KISS_PROCESSOR_H

#include <Arduino.h>
#include <DebugLog.h>
#include <memory>
#include <vector>

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.hpp>

namespace Kiss {

class Processor {
  
protected:
  // Enum to represent special KISS markers
  enum Marker {
    Fend = 0xc0,
    Fesc = 0xdb,
    Tfend = 0xdc,
    Tfesc = 0xdd
  };

  // Enum to represent the state of the processor
  enum State {
    GetStart = 0,
    GetEnd,
    GetCmd,
    GetData,
    GetP,
    GetSlotTime,
    Escape
  };

  // Enum to represent KISS commands
  enum Cmd {
    Data = 0x00,
    TxDelay = 0x01,
    P = 0x02,
    SlotTime = 0x03,
    TxTail = 0x04,
    SetHardware = 0x06,
    SignalReport = 0x07,
    RebootRequested = 0x08,
    Telemetry = 0x09,
    NoCmd = 0x80
  };

  // Enum to represent the type of data being processed
  enum DataType {
      Raw = 0,
      Control,
      Reboot,
      None = 0x80
  };

  // Compile-time constants for configuration
  static constexpr int CfgToSerialDelayMs = 10;
  static constexpr int CfgSerialToRigQueueSize = 4096;
  static constexpr int CfgRigToSerialQueueSize = 4096;

public:
  Processor();
  virtual ~Processor() = default; // Add virtual destructor

  // Sends data from rig to serial with a specific command
  void sendRigToSerial(Cmd cmd, const byte *packet, int packetLength);

  // Queues data for transmission from rig to serial
  void queueRigToSerial(Cmd cmd, const byte *packet, int packetLength);

  // Queues data for transmission from serial to rig
  void queueSerialToRig(Cmd cmd, const byte *packet, int packetLength);

  // Processes queued data for transmission from rig to serial
  bool processRigToSerial();

  // Processes queued data for transmission from serial to rig
  bool processSerialToRig();

protected:
  // Virtual methods to be implemented by derived classes
  virtual bool onRigTxBegin() = 0;
  virtual void onRigTx(byte b) = 0;
  virtual void onRigTxEnd() = 0;
  virtual void onRigPacket(void *packet, int packetLength) = 0;
  virtual void onSerialTx(byte b) = 0;
  virtual void onSerialTxEnd() = 0;
  virtual bool onSerialRxHasData() = 0;
  virtual bool onSerialRx(byte *b) = 0;
  virtual void onControlCommand(Cmd cmd, byte value) = 0;
  virtual void onRadioControlCommand(const std::vector<byte> &command) = 0;
  virtual void onRebootCommand() = 0;

private:
  // Processes a received byte
  bool receiveByte(byte rxByte);
  bool receiveByteRaw(byte rxByte); 
  bool receiveByteKiss(byte rxByte);

  // Processes data and commands
  void processData(byte rxByte);
  bool processCommand(byte rxByte);

protected:
  bool disableKiss_; // Flag to disable KISS mode
  bool usePrefix3_;  // Flag to use a 3-byte prefix

private:
  bool isRawIdle_;   // Indicates if raw mode is idle
  State state_;      // Current state of the processor
  DataType dataType_;// Current data type being processed
  std::vector<byte> cmdBuffer_; // Buffer for commands

  // Circular buffers for data queues
  CircularBuffer<uint8_t, CfgSerialToRigQueueSize> serialToRigQueue_;
  CircularBuffer<uint8_t, CfgRigToSerialQueueSize> rigToSerialQueue_;
  CircularBuffer<uint8_t, CfgRigToSerialQueueSize> rigToSerialQueueIndex_;
};

} // Kiss

#endif // KISS_PROCESSOR_H
