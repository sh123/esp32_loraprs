#include <arduino-timer.h>
#include "WiFi.h"
#include "loraprs_service.h"

#define LED_BUILTIN         2
#define LED_TOGGLE_PERIOD   1000

LoraPrs::Config cfg;
LoraPrs::Service loraPrsService;

auto watchdogLedTimer = timer_create_default();

void initializeConfig() {
  
  // client/server mode switch
  cfg.IsClientMode = false;

  // lora parameters
  cfg.LoraFreq = 433.775E6;
  cfg.LoraBw = 125e3;
  cfg.LoraSf = 12;
  cfg.LoraCodingRate = 7;
  cfg.LoraSync = 0x34;
  cfg.LoraPower = 20;

  // aprs configuration
  cfg.AprsHost = "rotate.aprs2.net";
  cfg.AprsPort = 14580;
  cfg.AprsLogin = "NOCALL-10";
  cfg.AprsPass = "12345";
  cfg.AprsFilter = "r/35.60/139.80/25"; // multiple filters are space separated
  cfg.AprsRawBeacon = "NOCALL-10>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0x34";
  cfg.AprsRawBeaconPeriodMinutes = 20;

  // bluetooth device name
  cfg.BtName = "loraprs";

  // server mode wifi paramaters
  cfg.WifiSsid = "<wifi ssid>";
  cfg.WifiKey = "<wifi key>";

  // configuration flags and features
  cfg.EnableAutoFreqCorrection = false;  // automatic tune to any incoming packet frequency
  cfg.EnableSignalReport = true;  // signal report will be added to the comment sent to aprsis
  cfg.EnablePersistentAprsConnection = true; // keep aprsis connection open, otherwise connect on new data only
  cfg.EnableRfToIs = true;  // send data from rf to aprsis
  cfg.EnableIsToRf = false; // send data from aprsis to rf
  cfg.EnableRepeater = false; // digirepeat incoming packets
  cfg.EnableBeacon = false;  // enable periodic AprsRawBeacon beacon to rf and aprsis if rf to aprsis is enabled
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  Serial.begin(115200);
  while (!Serial);

  initializeConfig();
  loraPrsService.setup(cfg);

  watchdogLedTimer.every(LED_TOGGLE_PERIOD, toggleWatchdogLed);
}

void loop() {
  loraPrsService.loop();
  watchdogLedTimer.tick();
}

bool toggleWatchdogLed(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  return true;
}
