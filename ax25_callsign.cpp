#include "ax25_callsign.h"

namespace AX25 {

Callsign::Callsign()
  : isValid_(false)
  , call_()
  , ssid_(0)
{
}

Callsign::Callsign(const Callsign &callsign)
  : isValid_(callsign.isValid_)
  , call_(callsign.call_)
  , ssid_(callsign.ssid_)
{
}

Callsign& Callsign::operator=(const Callsign &callsign)
{
  isValid_ = callsign.isValid_;
  call_ = callsign.call_;
  ssid_ = callsign.ssid_;
  return *this;
}

Callsign::Callsign(const byte *rxPayload, int payloadLength)
  : isValid_(false)
  , call_()
  , ssid_(0)
{
  isValid_ = fromBinary(rxPayload, payloadLength);
}

Callsign::Callsign(const String &callsign)
  : isValid_(false)
  , call_()
  , ssid_(0)
{
  isValid_ = fromString(callsign);
}

bool Callsign::ToBinary(byte *txPayload, int bufferLength) const
{
  if (bufferLength < CallsignSize) return false;

  byte *ptr = txPayload;

  memset(ptr, 0, bufferLength);

  for (int i = 0; i < CallsignSize - 1; i++) {
    if (i < call_.length()) {
      char c = call_.charAt(i);
      *(ptr++) = c << 1;
    }
    else {
      *(ptr++) = char(' ') << 1;
    }
  }
  *(txPayload + CallsignSize - 1) = ssid_ << 1;

  return true;
}

String Callsign::ToString() const
{
  String result = call_;
  if (ssid_ != 0) {
    result += "-" + String(ssid_);
  }
  return result;
}

bool Callsign::Digirepeat()
{
  if (IsPath()) {
    if (ssid_ > 0) {
      if (--ssid_ == 0) {
        call_ += "*";
      }
      return true;
    }
  }
  return false;
}

bool Callsign::fromString(const String &inputCallsign) 
{
  String callsign = inputCallsign;
  
  // "ABCDEF-XX"
  if (callsign.length() > CallsignSize + 2 || callsign.length() == 0) return false;

  int delimIndex = callsign.indexOf('-');

  // "ABCDEF-"
  if (delimIndex != -1 && delimIndex == callsign.length() - 1) return false;

  call_ = callsign;
  ssid_ = 0;

  if (delimIndex == -1) {
    // "ABCDEF"
    if (call_.length() >= CallsignSize) return false;
  }
  else {
    call_ = callsign.substring(0, delimIndex);
    ssid_ = callsign.substring(delimIndex + 1).toInt();
  }

  return true;
}

bool Callsign::fromBinary(const byte *rxPtr, int length)
{
  if (length < CallsignSize) return false;

  byte callsign[CallsignSize];

  const byte *ptr = rxPtr;

  memset(callsign, 0, sizeof(callsign));

  for (int i = 0; i < CallsignSize - 1; i++) {
    char c = *(ptr++) >> 1;
    callsign[i] = (c == ' ') ? '\0' : c;
  }
  callsign[CallsignSize-1] = '\0';

  ssid_ = (*ptr >> 1) & 0x0f;
  call_ = String((char*)callsign);

  if (call_.length() == 0) return false;

  return true;
}

} // AX25
