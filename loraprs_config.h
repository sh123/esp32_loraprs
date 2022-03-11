#ifndef LORAPRS_CONFIG_H
#define LORAPRS_CONFIG_H

#include <Arduino.h>
#include <DebugLog.h>

namespace LoraPrs {
  
struct Config
{
  DebugLogLevel LogLevel;    // log level
  bool IsClientMode;    // false - server mode, true - client mode (disables wifi and aprsis)

  // lora protocol parameters
  long LoraFreqRx;      // lora RX frequency, e.g. 433.775e6
  long LoraFreqTx;      // lora TX frequency, e.g. 433.775e6
  long LoraBw;          // lora bandwidth, e.g. 125e3
  int LoraSf;           // lora spreading factor, e.g. 12
  int LoraCodingRate;   // lora coding rate, e.g. 7
  int LoraPower;        // lora power level in dbm, 20
  int LoraSync;         // lora sync word/packet id, 0x34
  int LoraCrc;          // lora crc mode, 0 - disabled, 1 - 1 byte, 2 - 2 bytes
  bool LoraExplicit;    // lora header mode, true - explicit, false - implicit

  // lora hardware pinouts and isr
  byte LoraPinSs;       // lora ss pin
  byte LoraPinRst;      // lora rst pin
  byte LoraPinA;        // (sx127x - dio0, sx126x/sx128x - dio1)
  byte LoraPinB;        // (sx127x - dio1, sx126x/sx128x - busy)
  byte LoraPinSwitchRx; // (sx127x - unused, sx126x - RXEN pin number)
  byte LoraPinSwitchTx; // (sx127x - unused, sx126x - TXEN pin number)
  bool LoraUseIsr;      // true to use interrupts, false for fallback polling, e.g. if Dio0 is not connected
  bool LoraUseCad;      // use carrier detect before transmitting

  // usb
  bool UsbSerialEnable; // true - operate in USB Serial mode, debug logging is disabled
  
  // bluetooth
  String BtName;        // bluetooth device name for the client, set to empty string to disable bluetooth in server mode
  bool BtEnableBle;     // bluetooth device presents as BLE rather than serial bluetooth e.g. for iOS devices

  // wifi
  bool WifiEnableAp;    // true to run as access point
  String WifiSsid;      // wifi access point name
  String WifiKey;       // wifi access point key

  // kiss
  bool KissEnableTcpIp; // true to enable KISS over TCP/IP as a server
  bool KissEnableExtensions; // true - enable kiss extensions for radio control and signal reports

  // aprsis connectivity
  int AprsPort;         // aprs server port, 14580
  String AprsHost;      // aprs server hostname, rotate.aprs2.net
  String AprsLogin;     // aprs callsign to use, e.g. N0CALL-1
  String AprsPass;      // aprs login password
  String AprsFilter;    // aprs filter, see http://www.aprs-is.net/javAPRSFilter.aspx, do not include filter directive, just space separated values
  String AprsRawBeacon; // aprs string for server beacon, e.g. NOCALL-1>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0xf3
  int AprsRawBeaconPeriodMinutes; // aprs beacon period

  // frequency correction
  bool EnableAutoFreqCorrection;        // true - correct own frequency based on received packet frequency deviation
  int AutoFreqCorrectionDeltaHz;        // correct when difference is larger than this value
  
  // aprs logic
  bool EnableSignalReport;              // true - append signal report on server side for the client to be sent to APRS-IS
  bool EnablePersistentAprsConnection;  // true - keep aprs-is connection active all the time
  bool EnableRfToIs;                    // true - enable RF to APRS-IS submission
  bool EnableIsToRf;                    // true - enable APRS-IS to RF submission
  bool EnableRepeater;                  // true - digirepeat incoming packets based on WIDEn-n paths
  bool EnableBeacon;                    // true - send AprsRawBeacon to RF and APRS-IS if EnableRfToIs is true
  bool EnableTextPackets;               // true - use text plain messages insead of AX25 binary frames for interoperability with other projects

  // external ptt tx control
  bool PttEnable;                       // true - enable external ptt control
  int PttPin;                           // esp pin to set high on transmit
  int PttTxDelayMs;                     // ptt tx delay
  int PttTxTailMs;                      // ptt tx tail
};

} // LoraPrs

#endif // LORAPRS_CONFIG_H
