#include <arduino-timer.h>

#define DEBUGLOG_DEFAULT_LOG_LEVEL_INFO
#include <DebugLog.h>

#include "config.h"

#if CFG_IS_CLIENT_MODE == true
#pragma message("Configured for client mode")
#else
#pragma message("Configured for server mode")
#endif

#include "loraprs_service.h"

const int CfgPollDelayMs = 10;              // main loop delay

/*
 * Initialize config from config.h options.
 * Enables future use of EEPROM or allows user to dynamically modify config at run time.
 */
void initializeConfig(LoraPrs::Config &cfg) {

  // log level
  cfg.LogLevel = (DebugLogLevel)CFG_LOG_LEVEL;
  
  // client/server mode switch
  cfg.IsClientMode = CFG_IS_CLIENT_MODE;

  // modulation
  cfg.ModType = CFG_MOD_TYPE;

  // generic module parameters
  cfg.LoraFreqRx = CFG_LORA_FREQ_RX;
  cfg.LoraFreqTx = CFG_LORA_FREQ_TX;
  cfg.LoraPower = CFG_LORA_PWR;

  // lora parameters, must match on devices
  cfg.LoraBw = CFG_LORA_BW;
  cfg.LoraSf = CFG_LORA_SF;
  cfg.LoraCodingRate = CFG_LORA_CR;
  cfg.LoraSync = CFG_LORA_SYNC;
  cfg.LoraCrc = CFG_LORA_CRC; // set to 0 to disable
  cfg.LoraExplicit = CFG_LORA_EXPLICIT;
  cfg.LoraPreamble = CFG_LORA_PREAMBLE;

  // fsk parameters
  cfg.FskBitRate = CFG_FSK_BIT_RATE;
  cfg.FskFreqDev = CFG_FSK_FREQ_DEV;
  cfg.FskRxBw = CFG_FSK_RX_BW;

  // lora pinouts
  cfg.LoraPinSs = CFG_LORA_PIN_NSS;
  cfg.LoraPinRst = CFG_LORA_PIN_RST;
  cfg.LoraPinA = CFG_LORA_PIN_DIO1; // (sx127x - dio0, sx126x/sx128x - dio1)
  cfg.LoraPinB = CFG_LORA_PIN_BUSY; // (sx127x - dio1, sx126x/sx128x - busy)
  cfg.LoraPinSwitchRx = CFG_LORA_PIN_RXEN;  // (sx127x - unused, sx126x - RXEN pin number)
  cfg.LoraPinSwitchTx = CFG_LORA_PIN_TXEN;  // (sx127x - unused, sx126x - TXEN pin number)
  cfg.LoraUseCad = CFG_LORA_USE_CAD;  // carrier detect

  // aprs configuration
  cfg.AprsHost = "rotate.aprs2.net";
  cfg.AprsPort = 14580;
  cfg.AprsLogin = CFG_APRS_LOGIN;
  cfg.AprsPass = CFG_APRS_PASS;
  cfg.AprsFilter = CFG_APRS_FILTER; // multiple filters are space separated
  cfg.AprsRawBeacon = CFG_APRS_RAW_BKN;
  cfg.AprsRawBeaconPeriodMinutes = 20;

  // USB
  cfg.UsbSerialEnable = CFG_USB_SERIAL_ENABLE;

  // bluetooth device name
  cfg.BtName = CFG_BT_NAME;
  cfg.BtEnableBle = CFG_BT_USE_BLE;
  cfg.BtPassKey = CFG_BT_PASSKEY;
  cfg.BtBlePwr = CFG_BT_BLE_PWR;

  // server mode wifi paramaters
  cfg.WifiEnableAp = CFG_WIFI_ENABLE_AP;
  cfg.WifiSsid = CFG_WIFI_SSID;
  cfg.WifiKey = CFG_WIFI_KEY;

  // frequency correction
  cfg.EnableAutoFreqCorrection = CFG_FREQ_CORR;  // automatic tune to any incoming packet frequency
  cfg.AutoFreqCorrectionDeltaHz = CFG_FREQ_CORR_DELTA;

  // configuration flags and features
  cfg.EnableSignalReport = CFG_SIGNAL_REPORT;  // signal report will be added to the comment sent to aprsis
  cfg.EnablePersistentAprsConnection = CFG_PERSISTENT_APRS; // keep aprsis connection open, otherwise connect on new data only
  cfg.EnableRfToIs = CFG_RF_TO_IS;  // send data from rf to aprsis
  cfg.EnableIsToRf = CFG_IS_TO_RF; // send data from aprsis to rf
  cfg.EnableRepeater = CFG_DIGIREPEAT; // digirepeat incoming packets
  cfg.EnableRepeaterRaw = CFG_DIGIREPEAT_RAW; // digirepeat raw packets
  cfg.EnableBeacon = CFG_BEACON;  // enable periodic AprsRawBeacon beacon to rf and aprsis if rf to aprsis is enabled
  cfg.EnableTextPackets = CFG_TEXT_PACKETS; // enables TNC2 text packets and disables KISS+AX25 binary frames for interoperability
  cfg.EnableTextPackets3 = CFG_TEXT_PACKETS_3; // enable aprs-lora 3 byte prefix '<', 0xff, 0x01

  // kiss
  cfg.KissEnableExtensions = CFG_KISS_EXTENSIONS; // radio control and signal reports
  cfg.KissEnableTcpIp = CFG_KISS_TCP_IP;  // enable KISS ovr TCP/IP as a server

  // external ptt control
  cfg.PttEnable = CFG_PTT_ENABLE;
  cfg.PttPin = CFG_PTT_PIN;
  cfg.PttTxDelayMs = CFG_PTT_TX_DELAY_MS;
  cfg.PttTxTailMs = CFG_PTT_TX_TAIL_MS;

  // battery level monitor
  cfg.TlmEnable = CFG_TLM_ENABLE;
  cfg.TlmBatMonPin = CFG_TLM_BAT_MON_PIN;
  cfg.TlmBatMonCal = CFG_TLM_BAT_MON_CAL;
}

LoraPrs::Service loraPrsService;

auto watchdogLedTimer = timer_create_default();

bool toggleWatchdogLed(void *) {
  #ifdef BUILTIN_LED
  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  #endif
  return true;
}

void setup() {
  #ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, 1);
  #endif

  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial);
  
  LoraPrs::Config config;

  initializeConfig(config);
  loraPrsService.setup(config);

  watchdogLedTimer.every(LED_TOGGLE_PERIOD, toggleWatchdogLed);
}

void loop() {
  loraPrsService.loop();
  watchdogLedTimer.tick();
  delay(CfgPollDelayMs);
}
