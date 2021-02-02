#ifndef KISS_PROCESSOR_H
#define KISS_PROCESSOR_H

#include <Arduino.h>
#include <cppQueue.h>
#include <memory>

namespace Kiss {

class Processor {

public:
  Processor();
  
  void serialSend(const byte *data, int dataLength);
  void serialProcessRx();

protected:
  enum Marker {
    Fend = 0xc0,
    Fesc = 0xdb,
    Tfend = 0xdc,
    Tfesc = 0xdd
  };

  enum State {
    Void = 0,
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
    Frequency = 0x10,
    Bandwidth = 0x11,
    Power = 0x12,
    SyncWord = 0x13,
    SpreadingFactor = 0x14,
    CodingRate = 0x15,
    EnableCrc = 0x16,
    
    // extended events from modem
    SignalLevel = 0x30,

    // end of cmds
    NoCmd = 0x80
  };

  const int CfgTxQueueSize = 4096;
    
protected:
  virtual bool onRigTxBegin() = 0;
  virtual void onRigTx(byte data) = 0;
  virtual void onRigTxEnd() = 0;

  virtual void onSerialTx(byte data) = 0;
  virtual bool onSerialRxHasData() = 0;
  virtual bool onSerialRx(byte *data) = 0;

  virtual void onControlCommand(Cmd cmd, byte value) = 0;
  /*
  virtual void onControlCommand(Cmd cmd, int value) = 0;
  virtual void onControlCommand(Cmd cmd, long value) = 0;
  */

private:
  bool receiveByte(unsigned char rxByte);
  bool processCommand(unsigned char rxByte);
  void resetState();
  
private:
  Cmd cmd_;
  State state_;
  std::shared_ptr<cppQueue> txQueue_;
};
  
} // Kiss

#endif // KISS_PROCESSOR_H
