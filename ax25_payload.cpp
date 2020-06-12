#include "ax25_payload.h"

namespace AX25 {
  
Payload::Payload(byte *rxPayload, int payloadLength)
{
  parsePayload(rxPayload, payloadLength);
}

String Payload::ToText(const String &customComment)
{
  String txt = srcCall_ + String(">") + dstCall_;

  for (int i = 0; i < rptCallsCount_; i++) {
    if (rptCalls_[i].length() > 0) {
      txt += String(",") + rptCalls_[i];
    }
  }
  
  txt += String(":") + info_;

  if (info_.startsWith("=")) {
    txt += customComment;
  }
  
  return txt + String("\n");
}

bool Payload::parsePayload(byte *rxPayload, int payloadLength)
{
  byte *rxPtr = rxPayload;
  byte *rxEnd = rxPayload + payloadLength;

  // destination address
  dstCall_ = decodeCall(rxPtr);
  rxPtr += CallsignSize;
  if (rxPtr >= rxPayload + payloadLength) return false;

  // source address
  srcCall_ = decodeCall(rxPtr);
  rxPtr += CallsignSize;
  if (rxPtr >= rxEnd) return false;

  rptCallsCount_ = 0;
  
  // digipeater addresses
  for (int i = 0; i < RptMaxCount; i++) {
    if ((rxPayload[(i + 2) * CallsignSize - 1] & 1) == 0) {
      rptCalls_[i] = decodeCall(rxPtr);
      rptCallsCount_++;
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

String Payload::decodeCall(byte *rxPtr)
{
  byte callsign[CallsignSize];
  char ssid;
  
  byte *ptr = rxPtr;

  memset(callsign, 0, sizeof(callsign));
    
  for (int i = 0; i < 6; i++) {
    char c = *(ptr++) >> 1;
    callsign[i] = (c == ' ') ? '\0' : c;
  }
  callsign[CallsignSize-1] = '\0';
  ssid = (*ptr >> 1);
  
  String result = String((char*)callsign);
  
  if (result.length() > 0 && ssid >= '0' && ssid <= '9') {
    result += String("-") + String(ssid);
  }
  return result;
}

} // AX25
