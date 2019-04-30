#include "WiFi.h"
#include "loraprs.h"

#define LORAPRS_CLIENT

#define LORAPRS_FREQ        432.5E6

#ifdef LORAPRS_CLIENT
#define LORAPRS_BT_NAME     "loraprs_client"
#define LORAPRS_WIFI_SSID   ""
#define LORAPRS_WIFI_KEY    ""
#define LORAPRS_LOGIN       "NOCALL-0"
#define LORAPRS_PASS        "00000"
#else
#define LORAPRS_BT_NAME     ""
#define LORAPRS_WIFI_SSID   "<mywifi>"
#define LORAPRS_WIFI_KEY    "<key>"
#define LORAPRS_LOGIN       "NOCALL-0"
#define LORAPRS_PASS        "00000"
#endif 

LoraPrs loraPrs(
  LORAPRS_FREQ, 
  LORAPRS_BT_NAME, 
  LORAPRS_WIFI_SSID, 
  LORAPRS_WIFI_KEY,
  LORAPRS_LOGIN,
  LORAPRS_PASS);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  loraPrs.setup();
}

void loop() {
  loraPrs.loop();
}
