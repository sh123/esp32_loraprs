#define CFG_IS_CLIENT_MODE    true

#ifdef BOARD_T_BEAM
#define CFG_LORA_PIN_SS       18
#define CFG_LORA_PIN_RST      23
#define CFG_LORA_PIN_DIO0     26
#else
#define CFG_LORA_PIN_SS       5
#define CFG_LORA_PIN_RST      26
#define CFG_LORA_PIN_DIO0     14
#endif

#define CFG_LORA_FREQ         433.775E6
#define CFG_LORA_BW           125e3
#define CFG_LORA_SF           12
#define CFG_LORA_CR           7
#define CFG_LORA_PWR          20
#define CFG_LORA_ENABLE_CRC   true

#define CFG_BT_NAME           "loraprs"

#define CFG_APRS_LOGIN        "NOCALL-10"
#define CFG_APRS_PASS         "12345"
#define CFG_APRS_FILTER	      "r/35.60/139.80/25"
#define CFG_APRS_RAW_BKN      "NOCALL-10>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0x34"

#define CFG_WIFI_SSID         "<ssid>"
#define CFG_WIFI_KEY          "<key>"

#define CFG_FREQ_CORR         false
#define CFG_PERSISTENT_APRS   true
#define CFG_DIGIREPEAT        false
#define CFG_RF_TO_IS          true
#define CFG_IS_TO_RF          false
#define CFG_BEACON            false
