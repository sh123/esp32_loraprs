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

#define CFG_IS_CLIENT_MODE    true      // false - server mode (APRS-IS gate mode)

// lora pinouts, CAD and ISR usage
#define CFG_LORA_PIN_SS       SS
#define CFG_LORA_PIN_RST      LORA_RST
#define CFG_LORA_PIN_DIO0     LORA_IRQ
#ifdef USE_RADIOLIB
#define CFG_LORA_PIN_DIO1     RADIOLIB_NC // set to your DIO1 pin number if connected
#define CFG_LORA_USE_ISR      true        // always ON for RadioLib
#else
#define CFG_LORA_PIN_DIO1     LORA_IRQ  // not used in arduino-LoRa
#define CFG_LORA_USE_ISR      false // set to true for ISR usage in arduino-LoRa
#endif
#define CFG_LORA_USE_CAD      false // set to true to utilize carrier detection

// lora protocol parameters
#define CFG_LORA_FREQ         433.775E6
#define CFG_LORA_BW           125e3
#define CFG_LORA_SF           12
#define CFG_LORA_CR           7
#define CFG_LORA_PWR          20
#define CFG_LORA_ENABLE_CRC   true  // set to false for speech data

// wifi client and AP options
#define CFG_WIFI_ENABLE_AP    false     // run as wifi access point, for CFG_KISS_TCP_IP mode
#define CFG_WIFI_SSID         "<ssid>"  // connect to SSID or run as this SSID in AP mode
#define CFG_WIFI_KEY          "<key>"   // wifi key

// bluetooth
#define CFG_BT_NAME           "loraprs"
#define CFG_BT_USE_BLE        false // set to true to use bluetooth low energy (for ios devices)

// KISS protocol options
#define CFG_KISS_EXTENSIONS   false   // true - enable modem control from application with KISS commands
#define CFG_KISS_TCP_IP       false   // true - run as KISS TCP/IP server, no bluetooth operations performed

// APRS-IS options, valid in when CFG_IS_CLIENT_MODE = false
#define CFG_APRS_LOGIN        "NOCALL-10"
#define CFG_APRS_PASS         "12345"
#define CFG_APRS_FILTER	      "r/35.60/139.80/25"
#define CFG_APRS_RAW_BKN      "NOCALL-10>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0x34"

// APRS-IS gateway options, valid in when CFG_IS_CLIENT_MODE = false
#define CFG_PERSISTENT_APRS   true    // keep tcp/ip connection open (lot of traffic), otherwise connect on new packet (very rare traffic)
#define CFG_DIGIREPEAT        false   // digirepeat incoming packets
#define CFG_RF_TO_IS          true    // forward packets from radio to internet
#define CFG_IS_TO_RF          false   // forward packets from internet to radio basedon CFG_APRS_FILTER
#define CFG_BEACON            false   // enable perdiodc beacon from CFG_APRS_RAW_BKN

// frequency correction for narrow band bandwidths
#define CFG_FREQ_CORR         false   // true - correct own frequency based on received packet
#define CFG_FREQ_CORR_DELTA   1000    // correct when frequency difference is larger than this value

// PTT control
#define CFG_PTT_ENABLE        false   // enable external ptt (relay) control (for amplifier)
#define CFG_PTT_PIN           12      // PTT pin
#define CFG_PTT_TX_DELAY_MS   50      // time between relay swited on and transmission
#define CFG_PTT_TX_TAIL_MS    10      // time between stop transmission and relay off
