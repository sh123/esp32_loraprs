// comment out for arduino-Lora usage
#define USE_RADIOLIB

// Uncomment for SX126X module usage
//#define USE_SX126X

// Check your module name at https://github.com/jgromes/RadioLib/wiki/Modules
#ifdef USE_SX126X
#define MODULE_NAME   SX1268
#else
#define MODULE_NAME   SX1278
#endif

// generic options
#define LED_TOGGLE_PERIOD     1000
#define SERIAL_BAUD_RATE      115200
#define CFG_LOG_LEVEL         DebugLogLevel::LVL_INFO

// change pinouts if not defined through native board LORA_* definitions
#ifndef LORA_RST
#pragma message("LoRa pin definitions are not found, redefining...")
#define LORA_RST              26
#define LORA_IRQ              14
#endif

// redefine LED if not defined in Arduino to have module heartbeat indication
#ifndef BUILTIN_LED
#pragma message("BUILDIN_LED is not found, defining as 2")
#define BUILTIN_LED           2
#endif

// select between client mode and APRS-IS gate mode
#define CFG_IS_CLIENT_MODE    true        // false - server mode (APRS-IS gate mode)

// lora generic pinouts
#define CFG_LORA_PIN_SS       SS
#define CFG_LORA_PIN_RST      LORA_RST
#define CFG_LORA_PIN_A        LORA_IRQ    // (sx127x - dio0, sx126x/sx128x - dio1)
#define CFG_LORA_PIN_B        RADIOLIB_NC // (sx127x - dio1, sx126x/sx128x - busy)

// lora pinouts, SX126X RXEN/TXEN usage
#ifdef USE_SX126X
#define CFG_LORA_PIN_RXEN     4           // (sx127x - unused, sx126x - RXEN pin number)
#define CFG_LORA_PIN_TXEN     5           // (sx127x - unused, sx126x - TXEN pin number)
#endif

// CAD and ISR usage selection
#ifdef USE_SX126X
#define CFG_LORA_USE_ISR      false       // reading in ISR does not work on sx126x
#define CFG_LORA_USE_CAD      true        // do not transmit if channel is busy
#else
#define CFG_LORA_USE_ISR      true        // true - read incoming data in ISR, false - do not read in ISR
#define CFG_LORA_USE_CAD      true        // set to true to utilize carrier detection
#endif

// lora protocol default parameters (they need to match between devices!!!)
#define CFG_LORA_FREQ         433.775e6   // frequency in MHz
#define CFG_LORA_BW           125e3       // bandwidth (from 7.8 kHz up to 500 kHz)
#define CFG_LORA_SF           12          // spreading factor (7 - 12)
#define CFG_LORA_CR           7           // coding rate (5 - 8)
#define CFG_LORA_CRC          1           // 0 - disabled, 1 - 1 byte, 2 - 2 bytes
#define CFG_LORA_EXPLICIT     true        // header mode, true - explicit, false - implicit
#define CFG_LORA_SYNC         0x34        // sync word (0x12 - private, 0x34 - public)
#define CFG_LORA_PWR          20          // output power in dBm

// wifi client and AP options
#define CFG_WIFI_ENABLE_AP    false       // run as wifi access point (for CFG_KISS_TCP_IP mode)
#define CFG_WIFI_SSID         "<ssid>"    // connect to SSID or run as this SSID in AP mode
#define CFG_WIFI_KEY          "<key>"     // wifi key

// bluetooth
#define CFG_BT_NAME           "loraprs"
#define CFG_BT_USE_BLE        false // set to true to use bluetooth low energy (for ios devices)

// USB
#define CFG_USB_SERIAL_ENABLE false       // true - enable communication over USB Serial

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
#define CFG_TEXT_PACKETS      false   // enable aprs text packets instead of binary for interoperability with other projects (disables KISS+ax.25)

// frequency correction for narrow band bandwidths
#define CFG_FREQ_CORR         false   // true - correct own frequency based on received packet
#define CFG_FREQ_CORR_DELTA   1000    // correct when frequency difference is larger than this value

// PTT control
#define CFG_PTT_ENABLE        false   // enable external ptt (relay) control (for amplifier)
#define CFG_PTT_PIN           12      // PTT pin
#define CFG_PTT_TX_DELAY_MS   50      // time between relay swited on and transmission
#define CFG_PTT_TX_TAIL_MS    10      // time between stop transmission and relay off
