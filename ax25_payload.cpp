#include "ax25_payload.h"

namespace AX25 {
  
Payload::Payload(byte *rxPayload, int payloadLength)
{
  parsePayload(rxPayload, payloadLength);
}

String Payload::ToText(const String &customComment)
{
  String txt = srcCall_ + String(">") + dstCall_;
    
  if (rptFirst_.length() > 0) {
    txt += String(",") + rptFirst_;
  }
  if (rptSecond_.length() > 0) {
    txt += String(",") + rptSecond_;
  }

  txt += String(":") + body_;

  if (body_.startsWith("=")) {
    txt += customComment;
  }
  
  return txt + String("\n");
}

bool Payload::parsePayload(byte *rxPayload, int payloadLength)
{
  byte *rxPtr = rxPayload;

  dstCall_ = decodeCall(rxPtr);
  rxPtr += CallsignSize;
  if (rxPtr >= rxPayload + payloadLength) return false;

  srcCall_ = decodeCall(rxPtr);
  rxPtr += CallsignSize;
  if (rxPtr >= rxPayload + payloadLength) return false;
  
  if ((rxPayload[2 * CallsignSize - 1] & 1) == 0) {
    rptFirst_ = decodeCall(rxPtr);
    rxPtr += CallsignSize;
    if (rxPtr >= rxPayload + payloadLength) return false;

    if ((rxPayload[3 * CallsignSize - 1] & 1) == 0) {
      rptSecond_ = decodeCall(rxPtr);
      rxPtr += CallsignSize;
    }
  }
  if ((rxPtr + 1) >= rxPayload + payloadLength) return false;
  
  if (*(rxPtr++) != AX25Ctrl::UI) return false;
  if (*(rxPtr++) != AX25Pid::NoLayer3) return false;
  
  while (rxPtr < rxPayload + payloadLength) {
    body_ += String((char)*(rxPtr++));
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
