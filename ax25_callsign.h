#ifndef AX25_CALLSIGN_H
#define AX25_CALLSIGN_H

#include <Arduino.h>

namespace AX25 {
  
class Callsign
{
public:
  Callsign();
  Callsign(const Callsign &callsign);
  Callsign& operator=(const Callsign &callsign);
  
  Callsign(const byte *rxPayload, int payloadLength);
  Callsign(String inputText);

  inline bool IsValid() const { return isValid_; };
  inline bool IsTrace() const { return call_.startsWith("TRACE"); }
  
  String ToString() const;
  bool ToBinary(byte *txPayload, int bufferLength) const;

  bool Digirepeat();
  
private:
  bool encode(byte *txPtr, int bufferLength) const;

  bool fromString(String callsign);
  bool fromBinary(const byte *rxPtr, int payloadLength);

private:
  const int CallsignSize = 7;
  
private:
  bool isValid_;
  String call_;
  byte ssid_;
};

} // AX25

#endif // APRSMSG_H
