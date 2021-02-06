#ifndef KISS_PROCESSOR_H
#define KISS_PROCESSOR_H

#include <Arduino.h>
#include <cppQueue.h>
#include <memory>

namespace Kiss {

class Processor {

public:
  Processor();
  
  void serialSend(const byte *b, int dataLength);
  void serialProcessRx();

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

    // generic
    Data = 0x00,
    P = 0x02,
    SlotTime = 0x03,

    // extended to modem
    RadioControl = 0x10,

    // extended from modem
    RadioSignalLevel = 0x30,

    // end of cmds
    NoCmd = 0x80
  };

  enum DataType {
      Raw = 0,
      Control
  };
  
  const int CfgTxQueueSize = 4096;
    
protected:
  virtual bool onRigTxBegin() = 0;
  virtual void onRigTx(byte b) = 0;
  virtual void onRigTxEnd() = 0;

  virtual void onSerialTx(byte b) = 0;
  virtual bool onSerialRxHasData() = 0;
  virtual bool onSerialRx(byte *b) = 0;

  virtual void onControlCommand(Cmd cmd, byte value) = 0;
  virtual void onRadioControlCommand(const std::vector<byte> &command) = 0;

private:
  bool receiveByte(byte rxByte);
  void processData(byte rxByte);
  bool processCommand(byte rxByte);

private:
  State state_;
  DataType dataType_;
  std::shared_ptr<cppQueue> txQueue_;
  std::vector<byte> cmdBuffer_;
};
  
} // Kiss

#endif // KISS_PROCESSOR_H
