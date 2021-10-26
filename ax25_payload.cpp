#include "ax25_payload.h"

namespace AX25 {
  
Payload::Payload(const byte *rxPayload, int payloadLength)
  : rptCallsCount_(0)
{
  isValid_ = fromBinary(rxPayload, payloadLength);
}

Payload::Payload(const String &textPayload)
  : rptCallsCount_(0)
{
  isValid_ = fromString(textPayload);
}

Payload::Payload(const Payload &payload)
  : isValid_(payload.isValid_)
  , srcCall_(payload.srcCall_)
  , dstCall_(payload.dstCall_)
  , rptCallsCount_(payload.rptCallsCount_)
  , info_(payload.info_)
{
    for (int i = 0; i < rptCallsCount_; i++) {
      rptCalls_[i] = payload.rptCalls_[i];
    }
}

Payload& Payload::operator=(const Payload &payload)
{
  isValid_ = payload.isValid_;
  srcCall_ = payload.srcCall_;
  dstCall_ = payload.dstCall_;
  rptCallsCount_ = payload.rptCallsCount_;
  info_ = payload.info_;
  for (int i = 0; i < rptCallsCount_; i++) {
    rptCalls_[i] = payload.rptCalls_[i];
  }
  return *this;
}

void Payload::Dump() 
{
  LOG_INFO("valid: ", isValid_);
  LOG_INFO("src: ", srcCall_.ToString());
  LOG_INFO("dst: ", dstCall_.ToString());
  LOG_INFO("rpt: ");
  for (int i = 0; i < rptCallsCount_; i++) {
    LOG_INFO(rptCalls_[i].ToString());
  }
  LOG_INFO("info: ", info_);
}

int Payload::ToBinary(byte *txPayload, int bufferLength) const
{
  byte *txPtr = txPayload;
  byte *txEnd = txPayload + bufferLength;

  // destination address
  if (!dstCall_.ToBinary(txPtr, CallsignSize)) return 0;
  txPtr += CallsignSize;
  if (txPtr >= txEnd) return 0;

  // source address
  if (!srcCall_.ToBinary(txPtr, CallsignSize)) return 0;
  txPtr += CallsignSize;
  if (txPtr >= txEnd) return 0;
  
  // digipeater addresses
  for (int i = 0; i < rptCallsCount_; i++) {
    if (!rptCalls_[i].ToBinary(txPtr, CallsignSize)) return 0;
    txPtr += CallsignSize;
    if (txPtr >= txEnd) return 0;
  }
  *(txPtr - 1) |= 1;
  
  // control + protocol id
  if ((txPtr + 2) >= txEnd) return 0;
  *(txPtr++) = AX25Ctrl::UI;
  *(txPtr++) = AX25Pid::NoLayer3;

  // information field
  for (int i = 0; i < info_.length(); i++) {
    char c = info_.charAt(i);
    if (c == '\0') break;
    *(txPtr++) = c;
    if (txPtr >= txEnd) break;
  }

  return (int)(txPtr-txPayload);
}

String Payload::ToString(const String &customComment) const
{
  String txt = srcCall_.ToString() + String(">") + dstCall_.ToString();

  for (int i = 0; i < rptCallsCount_; i++) {
    if (rptCalls_[i].IsValid()) {
      txt += String(",") + rptCalls_[i].ToString();
    }
  }

  txt += String(":") + info_;

  if (info_.startsWith("=")) {
    txt += customComment;
  }

  return txt;
}

bool Payload::Digirepeat(const Callsign &ownCallsign)
{
  for (int i = 0; i < rptCallsCount_; i++) {   
    if (rptCalls_[i].Digirepeat()) {
      // if trace was digirepeated insert own callsign
      if (rptCalls_[i].IsTrace()) {
        InsertRptCallsign(ownCallsign, i);
      }
      return true;
    }
  }
  return false;
}

bool Payload::InsertRptCallsign(const Callsign &rptCallsign, int index)
{
  if (rptCallsCount_ >= RptMaxCount 
    || index >= RptMaxCount 
    || index >= rptCallsCount_) return false;

  for (int i = index; i < rptCallsCount_; i++) {
    rptCalls_[i + 1] = rptCalls_[i];
  }
  rptCalls_[index] = rptCallsign;
  rptCallsCount_++;
  return true;
}

bool Payload::fromBinary(const byte *rxPayload, int payloadLength)
{
  const byte *rxPtr = rxPayload;
  const byte *rxEnd = rxPayload + payloadLength;
  
  if (payloadLength < CallsignSize) return false;
  
  // destination address
  dstCall_ = AX25::Callsign(rxPtr, CallsignSize);
  if (!dstCall_.IsValid()) return false;
  rxPtr += CallsignSize;
  if (rxPtr >= rxEnd) return false;

  // source address
  srcCall_ = AX25::Callsign(rxPtr, CallsignSize);
  if (!srcCall_.IsValid()) return false;
  rxPtr += CallsignSize;
  if (rxPtr >= rxEnd) return false;

  rptCallsCount_ = 0;

  // digipeater addresses
  for (int i = 0, j = 0; i < RptMaxCount; i++) {
    if ((rxPayload[(i + 2) * CallsignSize - 1] & 1) == 0) {
      rptCalls_[j] = AX25::Callsign(rxPtr, CallsignSize);
      if (rptCalls_[j].IsValid()) {
        rptCallsCount_++;
        j++;
      }
      rxPtr += CallsignSize;
      if (rxPtr >= rxEnd) return false;
    }
    else {
      break;
    }
  }

  // control + protocol id
  if ((rxPtr + 2) >= rxEnd) return false;
  if (*(rxPtr++) != AX25Ctrl::UI) return false;
  if (*(rxPtr++) != AX25Pid::NoLayer3) return false;

  // information field
  while (rxPtr < rxEnd) {
    info_ += String((char)*(rxPtr++));
  }

  return true;
}

bool Payload::fromString(const String &textPayload)
{
  String inputText = textPayload;
  
  int rptIndex = inputText.indexOf('>');
  int infoIndex = inputText.indexOf(':');

  // bad format
  if (rptIndex == -1 || infoIndex == -1) return false;

  info_ = inputText.substring(infoIndex + 1);
  srcCall_ = AX25::Callsign(inputText.substring(0, rptIndex));
  if (!srcCall_.IsValid()) return false;
  String paths = inputText.substring(rptIndex + 1, infoIndex);
  
  rptCallsCount_ = 0;
  int index = 0;
  while (rptCallsCount_ <= RptMaxCount) {
    int nextIndex = paths.indexOf(',', index);
    String pathItem = paths.substring(index, nextIndex == -1 ? paths.length() : nextIndex);
    if (index == 0) {
      dstCall_ = AX25::Callsign(pathItem);
      if (!dstCall_.IsValid()) return false;
    }
    else {
      rptCalls_[rptCallsCount_] = AX25::Callsign(pathItem);
      if (rptCalls_[rptCallsCount_].IsValid()) {
        rptCallsCount_++;
      }
    }
    if (nextIndex == -1) break;
    index = nextIndex + 1;
  }
  
  return true;
}

} // AX25
