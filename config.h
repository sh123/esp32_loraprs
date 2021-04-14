#define LED_TOGGLE_PERIOD     1000

#define SERIAL_BAUD_RATE      115200

// change pinouts if not defined through native board LORA_* definitions
#ifndef LORA_RST
#pragma message("LoRa pin definitions are not found, redefining...")
#define LORA_RST              26
#define LORA_IRQ              14
#endif

#ifndef BUILTIN_LED
#pragma message("BUILDIN_LED is not found, defining as 2")
#define BUILTIN_LED           2
#endif

#define CFG_IS_CLIENT_MODE    true

#define CFG_LORA_PIN_SS       SS
#define CFG_LORA_PIN_RST      LORA_RST
#define CFG_LORA_PIN_DIO0     LORA_IRQ
#define CFG_LORA_USE_ISR      false // set to true for incoming packet ISR usage (stream mode, e.g. speech)

#define CFG_LORA_FREQ         433.775E6
#define CFG_LORA_BW           125e3
#define CFG_LORA_SF           12
#define CFG_LORA_CR           7
#define CFG_LORA_PWR          20
#define CFG_LORA_ENABLE_CRC   true  // set to false for speech data

#define CFG_BT_NAME           "loraprs"

#define CFG_APRS_LOGIN        "N7DMR-10"
#define CFG_APRS_PASS         "12345"
#define CFG_APRS_FILTER	      "r/35.60/139.80/25"
#define CFG_APRS_RAW_BKN      ""  //set RAW BKN to empty string to utilize GPS
#define CFG_APRS_SYMBOL_FIRST "L"
#define CFG_APRS_SYMBOL_SECOND "&"
#define CFG_APRS_COMMENTS     "LoRA 433.775MHz/BW125/SF12/CR7/0x34"
#define CFG_APRS_BEACONMINUTES 2
#define CFG_WIFI_SSID         "<ssid>"
#define CFG_WIFI_KEY          "<key>"

#define CFG_FREQ_CORR         false   // NB! incoming interrupts may stop working on frequent corrections when enabled
#define CFG_FREQ_CORR_DELTA   1000    //      test with your module before heavy usage

#define CFG_PERSISTENT_APRS   false
#define CFG_DIGIREPEAT        false
#define CFG_RF_TO_IS          false
#define CFG_IS_TO_RF          false
#define CFG_BEACON            true
#define CFG_KISS_EXTENSIONS   true


//GPS Configuration

#define T_BEAM_V10 
#define GPS_RX_PIN 34
#define GPS_TX_PIN 12
#define I2C_SDA 21
#define I2C_SCL 22
#define GPS_BAND_RATE      9600
