#include "kiss_processor.h"

namespace Kiss {

Processor::Processor()
  : disableKiss_(false)
  , usePrefix3_(false)
  , isRawIdle_(true)
  , state_(State::GetStart)
  , dataType_(DataType::None)
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
    onSerialTxEnd();
  }
}

void Processor::queueRigToSerial(Cmd cmd, const byte *packet, int packetLength) {
  for (int i = 0; i < packetLength; i++) {
    if (!rigToSerialQueue_.unshift(packet[i])) {
      LOG_WARN("Rig to serial queue is full!");
      return;
    }
  }
  if (!rigToSerialQueueIndex_.unshift(packetLength)) {
    LOG_WARN("Rig to serial index queue is full!");
    return;
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

    int readCnt = rxPacketSize;
    for (int i = 0, j = 0; i < readCnt; i++) {
      byte rxByte = rigToSerialQueue_.pop();
      if (disableKiss_) {   
        // filter out properietary identifier
        if (usePrefix3_) {
          if ((i == 0 && rxByte == '<') ||
              (i == 1 && rxByte == 0xff) ||
              (i == 2 && rxByte == 0x01)) 
          {
            rxPacketSize--;
            continue;
          }
        }
      }
      buf[j++] = rxByte;
    }
    sendRigToSerial(Cmd::Data, buf, rxPacketSize);
    onRigPacket(&buf, rxPacketSize);

    isProcessed = true;
    if (!rigToSerialQueueIndex_.isEmpty()) delay(CfgToSerialDelayMs);
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
    vTaskDelay(1);
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
    if (usePrefix3_) {
      onRigTx('<');
      onRigTx(0xff);
      onRigTx(0x01);
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
        if (dataType_ == DataType::Raw) {
          onRigTx((byte)Marker::Fend);
        } else if (dataType_ == DataType::Control) {
          cmdBuffer_.push_back((byte)Marker::Fend);
        }
        state_ = State::GetData;
      }
      else if (rxByte == Marker::Tfesc) {
        if (dataType_ == DataType::Raw) {
          onRigTx((byte)Marker::Fesc);
        } else if (dataType_ == DataType::Control) {
          cmdBuffer_.push_back((byte)Marker::Fesc);
        }
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
