#include "kiss_processor.h"

namespace Kiss {

CircularBuffer<uint8_t, Processor::CfgSerialToRigQueueSize> Processor::serialToRigQueue_;
CircularBuffer<uint8_t, Processor::CfgRigToSerialQueueSize> Processor::rigToSerialQueue_;
CircularBuffer<uint8_t, Processor::CfgRigToSerialQueueSize> Processor::rigToSerialQueueIndex_;

Processor::Processor()
  : disableKiss_(false)
  , isRawIdle_(true)
  , state_(State::GetStart)
{
}

void Processor::sendRigToSerial(Cmd cmd, const byte *packet, int packetLength) {
  if (disableKiss_) {
    for (int i = 0; i < packetLength; i++) {
      byte rxByte = packet[i];
      // TNC2 splits on \n
      if (rxByte == '\0') {
        onSerialTx('\n');
      } else {
        onSerialTx(rxByte);
      }
      // append \n for TNC2 compatibility if not there
      if (rxByte != '\n' && rxByte != '\0' && i == packetLength - 1) {
        onSerialTx('\n');
      }
    }
  } else {
    onSerialTx((byte)Marker::Fend);
    onSerialTx((byte)cmd);

    for (int i = 0; i < packetLength; i++) {
      byte rxByte = packet[i];
  
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
}

void ICACHE_RAM_ATTR Processor::queueRigToSerialIsr(Cmd cmd, const byte *packet, int packetLength) {
  if (!rigToSerialQueueIndex_.unshift(packetLength)) {
    LOG_WARN("Rig to serial queue is full!");
    return;
  }
  for (int i = 0; i < packetLength; i++) {
    if (!rigToSerialQueue_.unshift(packet[i])) {
      LOG_WARN("Rig to serial queue is full!");
      return;
    }
  }
}

void Processor::queueSerialToRig(Cmd cmd, const byte *packet, int packetLength) {
  bool result = 1;
  if (disableKiss_) {
    // TNC2, send as is, receiveByteRaw will deal with it
    for (int i = 0; i < packetLength; i++) {
      byte rxByte = packet[i];
      result &= serialToRigQueue_.unshift(rxByte);
    }
    result &= serialToRigQueue_.unshift('\n');
  } else {
    result &= serialToRigQueue_.unshift(Marker::Fend);
    result &= serialToRigQueue_.unshift(cmd);
  
    for (int i = 0; i < packetLength; i++) {
      byte rxByte = packet[i];
  
      if (rxByte == Marker::Fend) {
        result &= serialToRigQueue_.unshift(Marker::Fesc);
        result &= serialToRigQueue_.unshift(Marker::Tfend);
      }
      else if (rxByte == Marker::Fesc) {
        result &= serialToRigQueue_.unshift(Marker::Fesc);
        result &= serialToRigQueue_.unshift(Marker::Tfesc);
      }
      else {
        result &= serialToRigQueue_.unshift(rxByte);
      }
    }
  
    result &= serialToRigQueue_.unshift(Marker::Fend);
  }

  if (!result) {
    LOG_WARN("Serial to rig queue overflow!");
  }
}

bool Processor::processRigToSerial() 
{  
  bool isProcessed = false;

  if (rigToSerialQueueIndex_.isEmpty()) {
    return isProcessed;
  }

  while (!rigToSerialQueueIndex_.isEmpty()) {
    
    int rxPacketSize = rigToSerialQueueIndex_.pop();
    byte buf[rxPacketSize];

    for (int i = 0; i < rxPacketSize; i++) {
      buf[i] = rigToSerialQueue_.pop();
    }
    sendRigToSerial(Cmd::Data, buf, rxPacketSize);
    onRigPacket(&buf, rxPacketSize);

    isProcessed = true;
    yield();
  }
  return isProcessed;
}

bool Processor::processSerialToRig()
{
  bool allProcessed = false;

  while (onSerialRxHasData() || !serialToRigQueue_.isEmpty()) {
    byte rxByte;
    if (onSerialRxHasData()) {
      if (onSerialRx(&rxByte)) {
          if (!serialToRigQueue_.unshift(rxByte)) {
            LOG_WARN("Serial to rig buffer is full!");
          }
      }
    }
    if (!serialToRigQueue_.isEmpty()) {
      rxByte = serialToRigQueue_.pop();
      if (receiveByte(rxByte)) {
        allProcessed = true;
      } else {
        serialToRigQueue_.push(rxByte);
      }
    }
    yield();
  }
  return allProcessed;
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
    case Cmd::RebootRequested:
      state_ = State::GetData;
      dataType_ = DataType::Reboot;
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
      } else if (dataType_ == DataType::Reboot) {
        onRebootCommand();
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

bool Processor::receiveByteRaw(byte rxByte) 
{
  if (isRawIdle_) {
    if (!onRigTxBegin()) {
      return false;
    }
    isRawIdle_ = false;
  }
  // NOTE, TNC2 uses \n as a packet delimiter
  if (rxByte == '\n') {
    onRigTxEnd();
    isRawIdle_ = true;
  } else {
    onRigTx(rxByte);
  }
  return true;
}

bool Processor::receiveByteKiss(byte rxByte)
{
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

bool Processor::receiveByte(byte rxByte) {
  
  if (disableKiss_) {
    return receiveByteRaw(rxByte);
  }
  return receiveByteKiss(rxByte);
}

} // kiss
