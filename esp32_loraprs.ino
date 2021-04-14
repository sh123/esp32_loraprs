#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <u-blox_config_keys.h>
#include <u-blox_structs.h>

#include <SPI.h>
#include <arduino-timer.h>
#include "WiFi.h"
#include "loraprs_service.h"
#include <MicroNMEA.h> //https://github.com/stevemarple/MicroNMEA
char nmeaBuffer[100];
MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));
SFE_UBLOX_GNSS myGNSS;

#if __has_include("/tmp/esp32_loraprs_config.h")
#pragma message("Using external config")
#include "/tmp/esp32_loraprs_config.h"
#else
#pragma message("Using default config")
#include "config.h"
#endif

#if CFG_IS_CLIENT_MODE == true
#pragma message("Configured for client mode")
#else
#pragma message("Configured for server mode")
#endif

void initializeConfig(LoraPrs::Config &cfg) {
  
  // client/server mode switch
  cfg.IsClientMode = CFG_IS_CLIENT_MODE;

  // lora parameters
  cfg.LoraFreq = CFG_LORA_FREQ;
  cfg.LoraBw = CFG_LORA_BW;
  cfg.LoraSf = CFG_LORA_SF;
  cfg.LoraCodingRate = CFG_LORA_CR;
  cfg.LoraSync = 0x34;
  cfg.LoraPower = CFG_LORA_PWR;
  cfg.LoraEnableCrc = CFG_LORA_ENABLE_CRC; // set to false for speech streaming data

  // lora pinouts
  cfg.LoraPinSs = CFG_LORA_PIN_SS;
  cfg.LoraPinRst = CFG_LORA_PIN_RST;
  cfg.LoraPinDio0 = CFG_LORA_PIN_DIO0;
  cfg.LoraUseIsr = CFG_LORA_USE_ISR;  // set to true for incoming packet ISR usage (stream mode, e.g. speech)

  // aprs configuration
  cfg.AprsHost = "rotate.aprs2.net";
  cfg.AprsPort = 14580;
  cfg.AprsLogin = CFG_APRS_LOGIN;
  cfg.AprsPass = CFG_APRS_PASS;
  cfg.AprsFilter = CFG_APRS_FILTER; // multiple filters are space separated
  cfg.AprsRawBeacon = CFG_APRS_RAW_BKN;
  cfg.AprsRawBeaconPeriodMinutes = CFG_APRS_BEACONMINUTES;

  cfg.AprsSymbolFirst = CFG_APRS_SYMBOL_FIRST;
  cfg.AprsSymbolSecond = CFG_APRS_SYMBOL_SECOND;
  cfg.AprsComments = CFG_APRS_COMMENTS;

  // bluetooth device name
  cfg.BtName = CFG_BT_NAME;

  // server mode wifi paramaters
  cfg.WifiSsid = CFG_WIFI_SSID;
  cfg.WifiKey = CFG_WIFI_KEY;

  // frequency correction
  cfg.EnableAutoFreqCorrection = CFG_FREQ_CORR;  // automatic tune to any incoming packet frequency
  cfg.AutoFreqCorrectionDeltaHz = CFG_FREQ_CORR_DELTA;

  // configuration flags and features
  cfg.EnableSignalReport = true;  // signal report will be added to the comment sent to aprsis
  cfg.EnablePersistentAprsConnection = CFG_PERSISTENT_APRS; // keep aprsis connection open, otherwise connect on new data only
  cfg.EnableRfToIs = CFG_RF_TO_IS;  // send data from rf to aprsis
  cfg.EnableIsToRf = CFG_IS_TO_RF; // send data from aprsis to rf
  cfg.EnableRepeater = CFG_DIGIREPEAT; // digirepeat incoming packets
  cfg.EnableBeacon = CFG_BEACON;  // enable periodic AprsRawBeacon beacon to rf and aprsis if rf to aprsis is enabled
  cfg.EnableKissExtensions = CFG_KISS_EXTENSIONS; // radio control and signal reports
}

LoraPrs::Service loraPrsService;

auto watchdogLedTimer = timer_create_default();

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, 1);

  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial);
  
  LoraPrs::Config config;

  initializeConfig(config);
  loraPrsService.setup(config);
  //init GPS
  Serial.println("initGPS");
  Serial1.begin(GPS_BAND_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  delay(1500);
  if (myGNSS.begin(Serial1) == false) {
     Serial.println(F("Ublox GNSS not detected at default I2C address. Please check wiring."));
  }
  watchdogLedTimer.every(LED_TOGGLE_PERIOD, toggleWatchdogLed);
}

void loop() {
  loraPrsService.loop();
  watchdogLedTimer.tick();
}

bool toggleWatchdogLed(void *) {
  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  return true;
}
void setGPSInfo(String arr[]){
      
      myGNSS.checkUblox(); //See if new data is available. Process bytes as they come in.
      //Get GPS Info
      if (nmea.isValid() == true) {
        float latitude_mdeg = nmea.getLatitude() / 10000.;
        float longitude_mdeg = nmea.getLongitude() / 10000.;
        String latitude1 =  (latitude_mdeg > 0) ? String(latitude_mdeg)+"N" : String(latitude_mdeg)+"S";
        String longitude1 = (longitude_mdeg > 0) ? String(longitude_mdeg)+"E" : String(fabs(longitude_mdeg))+"W";
        arr[0]=latitude1;
        arr[1]=longitude1;
        Serial.print("Latitude (deg): ");
        Serial.println(arr[0]);
        Serial.print("Longitude (deg): ");
        Serial.println(arr[1]);
    } else {
        Serial.print("No Fix - ");
        Serial.print("Num. satellites: ");
        Serial.println(nmea.getNumSatellites());
        arr[0]="0";
        arr[1]="0";
    }
}


void SFE_UBLOX_GNSS::processNMEA(char incoming)
  {
      //Take the incoming char from the Ublox I2C port and pass it on to the MicroNMEA lib
      //for sentence cracking
      nmea.process(incoming);
  }
