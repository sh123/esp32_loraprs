#include "ax25_payload.h"

namespace AX25 {
  
Payload::Payload(byte *rxPayload, int payloadLength)
{
  parsePayload(rxPayload, payloadLength);
}

Payload::Payload(String inputText)
{
  parseString(inputText);
}

bool ToBinary(byte *txPayload, int bufferLength)
{
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

bool Payload::parseString(String inputText)
{
  int rptIndex = inputText.indexOf('>');
  int infoIndex = inputText.indexOf(':');

  // bad format
  if (rptIndex == -1 || infoIndex == -1) return false;
  
  info_ = inputText.substring(infoIndex + 1);
  srcCall_ = inputText.substring(0, rptIndex);
  
  String paths = inputText.substring(rptIndex + 1, infoIndex);

  rptCallsCount_ = 0;
  int index = 0;
  while (index != -1 && rptCallsCount_ <= RptMaxCount) {
    int nextIndex = paths.indexOf(',', index);
    String pathItem = paths.substring(index == 0 ? index : index + 1, nextIndex == -1 ? paths.length() - 1 : nextIndex);
    if (index == 0) {
      dstCall_ = pathItem;
    }
    else {
      rptCallsCount_++;
      rptCalls_[rptCallsCount_ - 1] = pathItem;
    }
    index = nextIndex;
  }

  return true;
}

bool Payload::encodeCall(String inputText, byte *txPtr, int bufferLength)
{
  if (bufferLength < CallsignSize) return false;

  String callsign = inputText;
  byte ssid = 0;

  int delimIndex = inputText.indexOf('-');
  if (delimIndex + 1 >= inputText.length()) return false;

  if (delimIndex != -1) {
    callsign = inputText.substring(0, delimIndex);
    ssid = inputText.substring(delimIndex + 1).toInt();
  }

  byte *ptr = txPtr;

  memset(ptr, 0, bufferLength);

  for (int i = 0; i < CallsignSize - 1; i++) {
    if (i < callsign.length()) {
      char c = callsign.charAt(i);
      *(ptr++) = c << 1;
    }
    else {
      *(ptr++) = char(' ') << 1;
    }
  }
  *(txPtr + CallsignSize - 1) = ssid << 1;

  return true;
}

String Payload::decodeCall(byte *rxPtr)
{
  byte callsign[CallsignSize];
  
  byte *ptr = rxPtr;

  memset(callsign, 0, sizeof(callsign));
    
  for (int i = 0; i < CallsignSize - 1; i++) {
    char c = *(ptr++) >> 1;
    callsign[i] = (c == ' ') ? '\0' : c;
  }
  callsign[CallsignSize-1] = '\0';
  byte ssid = (*ptr >> 1) & 0x0f;
  
  String result = String((char*)callsign);
  
  if (result.length() > 0) {
    result += String("-") + String(ssid);
  }
  return result;
}

} // AX25
