#include "kiss_processor.h"

namespace Kiss {
  
Processor::Processor()
  : cmd_(Cmd::NoCmd)
  , state_(State::Void)
  , txQueue_(new cppQueue(sizeof(unsigned char), CfgTxQueueSize))
{
}

void Processor::serialSend(const byte *data, int dataLength) {
  onSerialTx((byte)Marker::Fend);
  onSerialTx((byte)Cmd::Data);

  for (int i = 0; i < dataLength; i++) {
    byte rxByte = data[i];

    if (rxByte == Marker::Fend) {
      onSerialTx((byte)Marker::Fesc);
      onSerialTx((byte)Marker::Tfend);
    }
    else if (rxByte == Marker::Fesc) {
      onSerialTx((byte)Marker::Fesc);
      onSerialTx((byte)Marker::Tfesc);
    }
    else {
      onSerialTx(rxByte);
    }
  }

  onSerialTx((byte)Marker::Fend);
}
  
void Processor::serialProcessRx() 
{
  while (onSerialRxHasData() || !txQueue_->isEmpty()) {
    byte rxByte;
    if (onSerialRxHasData()) {      
      if (onSerialRx(&rxByte)) {
        if (!txQueue_->push((void *)&rxByte)) {
          Serial.println("TX queue is full");
          break;
        }
      }
    }
    if (!txQueue_->isEmpty()) {
      if (txQueue_->peek((void *)&rxByte)) {
        if (receiveByte(rxByte)) {
          txQueue_->drop();
        }
      }
    }
    yield();
  }
}

void Processor::resetState()
{
  cmd_ = Cmd::NoCmd;
  state_ = State::Void;
}

bool Processor::processCommand(unsigned char rxByte) {

  switch (rxByte) {
    case Cmd::Data:
      if (!onRigTxBegin()) return false;
      state_ = State::GetData;
      break;
    case Cmd::P:
      state_ = State::GetP;
      break;
    case Cmd::SlotTime:
      state_ = State::GetSlotTime;
      break;
    default:
      // unknown command
      resetState();
      return true;
  }
  cmd_ = (Cmd)rxByte;
  return true;
}

bool Processor::receiveByte(unsigned char rxByte) {

  switch (state_) {
    case State::Void:
      if (rxByte == Marker::Fend) {
        cmd_ = Cmd::NoCmd;
        state_ = State::GetCmd;
      }
      break;
    case State::GetCmd:
      if (rxByte != Marker::Fend) {
        if (!processCommand(rxByte)) return false;
      }
      break;
    case State::GetP:
      onControlCommand(Cmd::P, rxByte);
      state_ = State::GetData;
      break;
    case State::GetSlotTime:
      onControlCommand(Cmd::SlotTime, rxByte);
      state_ = State::GetData;
      break;
    case State::GetData:
      if (rxByte == Marker::Fesc) {
        state_ = State::Escape;
      }
      else if (rxByte == Marker::Fend) {
        if (cmd_ == Cmd::Data) {
          onRigTxEnd();
        }
        resetState();
      }
      else if (cmd_ == Cmd::Data) {
        onRigTx(rxByte);
      }
      break;
    case State::Escape:
      if (rxByte == Marker::Tfend) {
        onRigTx((byte)Marker::Fend);
        state_ = State::GetData;
      }
      else if (rxByte == Marker::Tfesc) {
        onRigTx((byte)Marker::Fesc);
        state_ = State::GetData;
      }
      else {
        resetState();
      }
      break;
    default:
      break;
  }
  return true;
}

} // kiss
