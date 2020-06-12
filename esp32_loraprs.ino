#include <timer.h>
#include "WiFi.h"
#include "loraprs.h"

#define LED_BUILTIN         2
#define LED_TOGGLE_PERIOD   1000

LoraPrsConfig cfg;
LoraPrs loraPrs;

auto watchdogLedTimer = timer_create_default();

void initializeConfig() {
  cfg.IsClientMode = true;
  
  cfg.LoraFreq = 433.775E6; // 433.7688E6;
  cfg.LoraBw = 125e3;
  cfg.LoraSf = 12;
  cfg.LoraCodingRate = 7;
  cfg.LoraSync = 0xf3;
  cfg.LoraPower = 20;

  cfg.AprsHost = "rotate.aprs2.net";
  cfg.AprsPort = 14580;
  cfg.AprsLogin = "NOCALL-1";
  cfg.AprsPass = "00000";
  cfg.AprsFilter = "r/35.60/139.80/25";
  
  cfg.BtName = "loraprs";
  
  cfg.WifiSsid = "<wifi ssid>";
  cfg.WifiKey = "<wifi key>";

  cfg.EnableSignalReport = true;
  cfg.EnableAutoFreqCorrection = true;
  cfg.EnablePersistentAprsConnection = true;
  cfg.EnableIsToRf = false;
  cfg.EnableRepeater = false;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  Serial.begin(115200);
  while (!Serial);

  initializeConfig();
  loraPrs.setup(cfg);

  watchdogLedTimer.every(LED_TOGGLE_PERIOD, toggleWatchdogLed);
}

void loop() {
  loraPrs.loop();
  watchdogLedTimer.tick();
}

bool toggleWatchdogLed(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  return true;
}
