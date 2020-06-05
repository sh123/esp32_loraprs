#include <timer.h>
#include "WiFi.h"
#include "loraprs.h"

#define LED_BUILTIN         2
#define LED_TOGGLE_PERIOD   1000

#define LORAPRS_CLIENT

// https://vienna.iaru-r1.org/wp-content/uploads/2019/01/VIE19-C5-015-OEVSV-LORA-APRS-433-MHz.pdf
#ifdef LORAPRS_CLIENT
// calibrate client based on server frequency drift report
#define LORAPRS_FREQ        433.775E6
//#define LORAPRS_FREQ        433.7688E6
#else
#define LORAPRS_FREQ        433.775E6
//#define LORAPRS_FREQ        433.770E6
#endif

#ifdef LORAPRS_CLIENT
#define LORAPRS_BT_NAME     "loraprs_client"
#define LORAPRS_WIFI_SSID   ""
#define LORAPRS_WIFI_KEY    ""
#define LORAPRS_LOGIN       "NOCALL-0"
#define LORAPRS_PASS        "00000"
#define LORAPRS_FREQ_CORR   false
#else
#define LORAPRS_BT_NAME     ""
#define LORAPRS_WIFI_SSID   "<your access point name>"
#define LORAPRS_WIFI_KEY    "<your access point key>"
#define LORAPRS_LOGIN       "NOCALL-0"
#define LORAPRS_PASS        "12345"
#define LORAPRS_FREQ_CORR   false
//#define LORAPRS_FREQ_CORR   true
#endif 

LoraPrs loraPrs(
  LORAPRS_FREQ, 
  LORAPRS_BT_NAME, 
  LORAPRS_WIFI_SSID, 
  LORAPRS_WIFI_KEY,
  LORAPRS_LOGIN,
  LORAPRS_PASS,
  LORAPRS_FREQ_CORR);

auto watchdogLedTimer = timer_create_default();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  Serial.begin(115200);
  while (!Serial);

  loraPrs.setup();

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
