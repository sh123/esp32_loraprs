#ifndef AX25_PAYLOAD_H
#define AX25_PAYLOAD_H

#include <Arduino.h>
#include <DebugLog.h>

#include "ax25_callsign.h"

namespace AX25 {
  
class Payload
{
public:
  Payload(const String &textPayload);
  Payload(const byte *rxPayload, int payloadLength);
  Payload(const Payload &payload);
  Payload& operator=(const Payload &payload);

  inline bool IsValid() const { return isValid_; }
  
  String ToString(const String &customComment=String()) const;
  int ToBinary(byte *txPayload, int bufferLength) const;

  bool Digirepeat(const Callsign &ownCallsign);
  void Dump();

private:
  bool fromString(const String &textPayload);
  bool fromBinary(const byte *rxPayload, int payloadLength);
  
  bool InsertRptCallsign(const Callsign &rptCallsign, int index);

private:
  enum AX25Ctrl {
    UI = 0x03
  };

  enum AX25Pid {
    NoLayer3 = 0xf0
  };

  const int CallsignSize = 7;
  const int RptMaxCount = 7;

private:
  bool isValid_;
  Callsign srcCall_, dstCall_;
  Callsign rptCalls_[7];
  int rptCallsCount_;
  String info_;
};

} // AX25

#endif // AX25_PAYLOAD_H
