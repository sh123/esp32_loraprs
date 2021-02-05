#include "kiss_processor.h"

namespace Kiss {
  
Processor::Processor()
  : cmd_(Cmd::NoCmd)
  , state_(State::Void)
  , txQueue_(new cppQueue(sizeof(unsigned char), CfgTxQueueSize))
{
}

void Processor::serialSend(const byte *b, int dataLength) {
  onSerialTx((byte)Marker::Fend);
  onSerialTx((byte)Cmd::Data);

  for (int i = 0; i < dataLength; i++) {
    byte rxByte = b[i];

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

bool Processor::processCommand(byte rxByte) {

  switch (rxByte) {
    case Cmd::Data:
      if (!onRigTxBegin()) return false;
      state_ = State::GetData;
      dataType_ = DataType::Raw;
      break;
    case Cmd::P:
      state_ = State::GetP;
      break;
    case Cmd::SlotTime:
      state_ = State::GetSlotTime;
      break;
    case Cmd::RadioControl:
      state_ = State::GetData;
      dataType_ = DataType::Control;
      cmdBuffer_.clear();
      break;
    default:
      // unknown command
      resetState();
      return true;
  }
  cmd_ = (Cmd)rxByte;
  return true;
}

void Processor::processData(byte rxByte) {
  switch (rxByte) {
    case Marker::Fesc:
      state_ = State::Escape;
      break;
    case Marker::Fend:
      if (cmd_ == Cmd::Data) {
        if (dataType_ == DataType::Raw) {
          onRigTxEnd();
        } else if (dataType_ == DataType::Control) {
          onRadioControlCommand(cmdBuffer_);
        }
      }
      resetState();
      break;
    default:
      if (cmd_ == Cmd::Data) {
        if (dataType_ == DataType::Raw) {
          onRigTx(rxByte);
        } else if (dataType_ == DataType::Control) {
          cmdBuffer_.push_back(rxByte);
        }
      }
      break;
  }
}

bool Processor::receiveByte(byte rxByte) {

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
      processData(rxByte);
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
