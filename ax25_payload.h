#ifndef APRSMSG_H
#define APRSMSG_H

#include <Arduino.h>

namespace AX25 {
  
class Payload
{
public:
  Payload(const byte *rxPayload, int payloadLength);
  Payload(const String &inputText);

  inline bool IsValid() const { return isValid_; }
  
  String ToText(const String &customComment) const;
  bool ToBinary(byte *txPayload, int bufferLength) const;

private:
  String decodeCall(const byte *rxPtr) const;
  bool encodeCall(const String &callsign, byte *txPtr, int bufferLength) const;

  bool parseString(const String &inputText);
  bool parsePayload(const byte *rxPayload, int payloadLength);
  
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
  String srcCall_, dstCall_;
  String rptCalls_[7];
  int rptCallsCount_;
  String info_;
};

} // AX25

#endif // APRSMSG_H
