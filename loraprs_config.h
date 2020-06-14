#ifndef LORAPRS_CONFIG_H
#define LORAPRS_CONFIG_H

#include <Arduino.h>

namespace LoraPrs {
  
struct Config
{
  bool IsClientMode;    // true - client mode, false - server mode
  
  long LoraFreq;        // lora frequency, e.g. 433.775e6
  int LoraBw;           // lora bandwidth, e.g. 125e3
  byte LoraSf;          // lora spreading factor, e.g. 12
  byte LoraCodingRate;  // lora coding rate, e.g. 7
  byte LoraSync;        // lora sync word/packet id, 0x3f
  byte LoraPower;       // lora power level in dbm, 20

  int AprsPort;         // aprs server port, 14580
  String AprsHost;      // aprs server hostname, rotate.aprs2.net
  String AprsLogin;     // aprs callsign to use, e.g. MY0CAL-1
  String AprsPass;      // aprs login password
  String AprsFilter;    // aprs filter, see http://www.aprs-is.net/javAPRSFilter.aspx
  String AprsRawBeacon; // aprs string for server beacon, e.g. NOCALL-1>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0xf3
  int AprsRawBeaconPeriodMinutes; // aprs beacon period
  
  String BtName;        // bluetooth device name for the client
  
  String WifiSsid;      // wifi access point name
  String WifiKey;       // wifi access point key

  bool EnableSignalReport;              // true - append signal report on server side for the client to be sent to APRS-IS
  bool EnableAutoFreqCorrection;        // true - correct own frequency based on received packet frequency deviation
  bool EnablePersistentAprsConnection;  // true - keep aprs-is connection active all the time
  bool EnableRfToIs;                    // true - enable RF to APRS-IS submission
  bool EnableIsToRf;                    // true - enable APRS-IS to RF submission
  bool EnableRepeater;                  // true - digirepeat incoming packets based on WIDEn-n paths
  bool EnableBeacon;                    // true - send AprsRawBeacon to RF and APRS-IS if EnableRfToIs is true
};

} // LoraPrs

#endif // LORAPRS_CONFIG_H
