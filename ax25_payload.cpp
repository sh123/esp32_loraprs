#include "ax25_payload.h"

namespace AX25 {
  
Payload::Payload(byte *rxPayload, int payloadLength)
  : rptCallsCount_(0)
{
  isValid_ = parsePayload(rxPayload, payloadLength);
}

Payload::Payload(String inputText)
  : rptCallsCount_(0)
{
  isValid_ = parseString(inputText);
}

void Payload::Dump() 
{
  Serial.println();
  Serial.print("valid: "); Serial.println(isValid_);
  Serial.println("src: " + srcCall_);
  Serial.println("dst: " + dstCall_);
  Serial.print("rpt: ");
  for (int i = 0; i < rptCallsCount_; i++) {
    Serial.print(rptCalls_[i] + " ");
  }
  Serial.println();
  Serial.println("info: " + info_);
}

int Payload::ToBinary(byte *txPayload, int bufferLength)
{
  byte *txPtr = txPayload;
  byte *txEnd = txPayload + bufferLength;

  // destination address
  if (!encodeCall(dstCall_, txPtr, CallsignSize)) return 0;
  txPtr += CallsignSize;
  if (txPtr >= txEnd) return 0;

  // source address
  if (!encodeCall(srcCall_, txPtr, CallsignSize)) return 0;
  txPtr += CallsignSize;
  if (txPtr >= txEnd) return 0;
  
  // digipeater addresses
  for (int i = 0; i < rptCallsCount_; i++) {
    if (!encodeCall(rptCalls_[i], txPtr, CallsignSize)) return 0;
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

String Payload::ToText(String customComment)
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

bool Payload::parsePayload(const byte *rxPayload, int payloadLength)
{
  const byte *rxPtr = rxPayload;
  const byte *rxEnd = rxPayload + payloadLength;

  // destination address
  dstCall_ = decodeCall(rxPtr);
  rxPtr += CallsignSize;
  if (rxPtr >= rxEnd) return false;

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
  while (rptCallsCount_ <= RptMaxCount) {
    int nextIndex = paths.indexOf(',', index);
    String pathItem = paths.substring(index, nextIndex == -1 ? paths.length() : nextIndex);
    if (index == 0) {
      dstCall_ = pathItem;
    }
    else {
      rptCallsCount_++;
      rptCalls_[rptCallsCount_ - 1] = pathItem;
    }
    if (nextIndex == -1) break;
    index = nextIndex + 1;
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

String Payload::decodeCall(const byte *rxPtr)
{
  byte callsign[CallsignSize];
  
  const byte *ptr = rxPtr;

  memset(callsign, 0, sizeof(callsign));
    
  for (int i = 0; i < CallsignSize - 1; i++) {
    char c = *(ptr++) >> 1;
    callsign[i] = (c == ' ') ? '\0' : c;
  }
  callsign[CallsignSize-1] = '\0';
  byte ssid = (*ptr >> 1) & 0x0f;
  
  String result = String((char*)callsign);
  
  if (result.length() > 0 && ssid != 0) {
    result += String("-") + String(ssid);
  }
  return result;
}

} // AX25
