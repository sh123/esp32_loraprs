#include "kiss_processor.h"

namespace Kiss {
  
Processor::Processor()
  : state_(State::GetStart)
  , txQueue_(new cppQueue(sizeof(unsigned char), CfgTxQueueSize))
{
}

void Processor::serialSend(Cmd cmd, const byte *b, int dataLength) {
  onSerialTx((byte)Marker::Fend);
  onSerialTx((byte)cmd);

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
    yield();
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

bool Processor::processCommand(byte rxByte) {

  switch (rxByte) {
    case Cmd::Data:
      if (!onRigTxBegin()) {
        return false;
      }
      state_ = State::GetData;
      dataType_ = DataType::Raw;
      break;
    case Cmd::SetHardware:
      state_ = State::GetData;
      dataType_ = DataType::Control;
      cmdBuffer_.clear();
      break;
    case Cmd::P:
      state_ = State::GetP;
      break;
    case Cmd::SlotTime:
      state_ = State::GetSlotTime;
      break;
    case Marker::Fend:
      break;
    default:
      // unknown command
      state_ = State::GetEnd;
      return true;
  }
  return true;
}

void Processor::processData(byte rxByte) {
  switch (rxByte) {
    case Marker::Fesc:
      state_ = State::Escape;
      break;
    case Marker::Fend:
      if (dataType_ == DataType::Raw) {
        onRigTxEnd();
      } else if (dataType_ == DataType::Control) {
        onRadioControlCommand(cmdBuffer_);
      }
      state_ = State::GetStart;
      break;
    default:
      if (dataType_ == DataType::Raw) {
        onRigTx(rxByte);
      } else if (dataType_ == DataType::Control) {
        cmdBuffer_.push_back(rxByte);
      }
      break;
  }
}

bool Processor::receiveByte(byte rxByte) {

  switch (state_) {
    case State::GetStart:
      if (rxByte == Marker::Fend) {
        state_ = State::GetCmd;
      }
      break;
    case State::GetEnd:
      if (rxByte == Marker::Fend) {
        state_ = State::GetStart;
      }
      break;
    case State::GetCmd:
      if (!processCommand(rxByte)) {
        return false;
      }
      break;
    case State::GetP:
      onControlCommand(Cmd::P, rxByte);
      state_ = State::GetEnd;
      break;
    case State::GetSlotTime:
      onControlCommand(Cmd::SlotTime, rxByte);
      state_ = State::GetEnd;
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
      else if (rxByte != Marker::Fend) {
        state_ = State::GetEnd;
      }
      break;
    default:
      // unknown state
      state_ = State::GetStart;
      break;
  }
  return true;
}

} // kiss
