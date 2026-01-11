#ifndef CONFIG_H
#define CONFIG_H

// below argumentrs could be overriden in board variant.h and/or platformio.ini file
#include "variant.h"

// generic options
#ifndef LED_TOGGLE_PERIOD
#define LED_TOGGLE_PERIOD     1000    // heartbeat LED period
#endif
#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE      115200  // USB serial baud rate
#endif

// USB serial logging
// set to DebugLogLevel::LVL_TRACE for packet logging
// set to DebugLogLevel::LVL_NONE to disable logging
#ifndef CFG_LOG_LEVEL
#define CFG_LOG_LEVEL         DebugLogLevel::LVL_INFO
#endif

// select between client mode and APRS-IS gate mode
#ifndef CFG_IS_CLIENT_MODE
#define CFG_IS_CLIENT_MODE    true        // false - server mode (APRS-IS gate mode)
#endif

// CAD and ISR usage selection
#ifndef CFG_LORA_USE_CAD
#define CFG_LORA_USE_CAD      true        // do not transmit if channel is busy
#endif

// modulation
#define CFG_MOD_TYPE_LORA     0
#define CFG_MOD_TYPE_FSK      1
#ifndef CFG_MOD_TYPE
#define CFG_MOD_TYPE          CFG_MOD_TYPE_LORA
#endif

// general radio parameters
#ifndef CFG_LORA_FREQ_RX
#define CFG_LORA_FREQ_RX      433.775e6   // RX frequency in MHz
#endif
#ifndef CFG_LORA_FREQ_TX
#define CFG_LORA_FREQ_TX      433.775e6   // TX frequency in MHz
#endif
#ifndef CFG_LORA_PWR
#define CFG_LORA_PWR          20          // output power in dBm
#endif

// LoRa protocol default parameters (they need to match between devices!!!)
#ifndef CFG_LORA_BW
#define CFG_LORA_BW           125e3       // bandwidth (from 7.8 kHz up to 500 kHz)
#endif
#ifndef CFG_LORA_SF
#define CFG_LORA_SF           12          // spreading factor (6 - 12), 6 requires implicit header mode
#endif
#ifndef CFG_LORA_CR
#define CFG_LORA_CR           7           // coding rate (5 - 8)
#endif
#ifndef CFG_LORA_CRC
#define CFG_LORA_CRC          1           // 0 - disabled, 1 - 1 byte, 2 - 2 bytes
#endif
#ifndef CFG_LORA_EXPLICIT
#define CFG_LORA_EXPLICIT     true        // header mode, true - explicit, false - implicit
#endif
#ifndef CFG_LORA_SYNC
#define CFG_LORA_SYNC         0x12        // sync word (0x12 - private used by other trackers, 0x34 - public used by LoRaWAN)
#endif
#ifndef CFG_LORA_PREAMBLE
#define CFG_LORA_PREAMBLE     8           // preamble length from 6 to 65535
#endif

// fsk modem default parameters (they need to match between devices!!!)
#ifndef CFG_FSK_BIT_RATE
#define CFG_FSK_BIT_RATE      4.8         // bit rate in Kbps from 0.6 to 300.0
#endif
#ifndef CFG_FSK_FREQ_DEV
#define CFG_FSK_FREQ_DEV      1.2         // frequency deviation in kHz from 0.6 to 200.0
#endif
#ifndef CFG_FSK_RX_BW  
#define CFG_FSK_RX_BW         9.7         // rx bandwidth in kHz !!discrete!! from 4.8 to 467.0
#endif

// WiFi client and AP options
#ifndef CFG_WIFI_ENABLE_AP    
#define CFG_WIFI_ENABLE_AP    false       // run as wifi access point (for CFG_KISS_TCP_IP mode)
#endif
#ifndef CFG_WIFI_SSID        
#define CFG_WIFI_SSID         "loraprs"   // connect to SSID or run as this SSID in AP mode
#endif
#ifndef CFG_WIFI_KEY        
#define CFG_WIFI_KEY          "123456"    // wifi key, always specified
#endif

// Bluetooth classic or BLE
#ifndef CFG_BT_NAME        
#define CFG_BT_NAME           "loraprs"   // BT/BLE dev and advert name
#endif
#ifndef CFG_BT_USE_BLE
#define CFG_BT_USE_BLE        true        // Use BLE LE by default, false sets to classic
#endif
#ifndef CFG_BT_BLE_PWR    
#define CFG_BT_BLE_PWR        0           // BLE power in dBm (0 dBm = 1mW, 10 dBm = 10mW, 20dBm = 100mW, etc)
#endif
#ifndef CFG_BT_PASSKEY   
#define CFG_BT_PASSKEY        123456      // passkey, integer, set to 0 to disable BLE passkey authorization
#endif

// USB serial
#ifndef CFG_USB_SERIAL_ENABLE 
#define CFG_USB_SERIAL_ENABLE false       // true - enable KISS communication over USB Serial (e.g. with APRSDroid over USB-OTG), disables USB logging
#endif

// KISS protocol options
#ifndef CFG_KISS_EXTENSIONS  
#define CFG_KISS_EXTENSIONS   true   // true - enable modem control from application with KISS commands and signal reports
#endif
#ifndef CFG_KISS_TCP_IP     
#define CFG_KISS_TCP_IP       false   // true - run as KISS TCP/IP server, no bluetooth operations performed
#endif

// APRS-IS options, valid in when CFG_IS_CLIENT_MODE = false
#ifndef CFG_APRS_LOGIN     
#define CFG_APRS_LOGIN        "NOCALL-10"
#endif
#ifndef CFG_APRS_PASS     
#define CFG_APRS_PASS         "12345"
#endif
#ifndef CFG_APRS_FILTER	 
#define CFG_APRS_FILTER	      "r/35.60/139.80/25"  // multiple are space separated, see http://www.aprs-is.net/javAPRSFilter.aspx
#endif
#ifndef CFG_APRS_RAW_BKN
#define CFG_APRS_RAW_BKN      "NOCALL-10>APZMDM,WIDE1-1:!0000.00N/00000.00E#LoRA 433.775MHz/BW125/SF12/CR7/0x12"
#endif

// APRS-IS gateway options, valid when CFG_IS_CLIENT_MODE = false
#ifndef CFG_SIGNAL_REPORT     
#define CFG_SIGNAL_REPORT     true    // include signal report into the comment when sending to APRS-IS
#endif
#ifndef CFG_PERSISTENT_APRS  
#define CFG_PERSISTENT_APRS   true    // keep tcp/ip connection open (lot of traffic), otherwise connect on new packet (very rare traffic)
#endif
#ifndef CFG_DIGIREPEAT      
#define CFG_DIGIREPEAT        false   // digirepeat incoming AX25 packets, with AX25 routing
#endif
#ifndef CFG_DIGIREPEAT_RAW 
#define CFG_DIGIREPEAT_RAW    false   // retransmit raw non-AX25 packets, e.g. speech without routing
#endif
#ifndef CFG_RF_TO_IS      
#define CFG_RF_TO_IS          true    // forward packets from radio to internet
#endif
#ifndef CFG_IS_TO_RF     
#define CFG_IS_TO_RF          false   // forward packets from internet to radio based on CFG_APRS_FILTER
#endif
#ifndef CFG_BEACON      
#define CFG_BEACON            false   // enable perdiodic beacon from CFG_APRS_RAW_BKN
#endif
#ifndef CFG_TEXT_PACKETS      
#define CFG_TEXT_PACKETS      false   // enable aprs TNC2 text packets instead of binary for interoperability with other projects (disables KISS + AX.25!)
#endif
#ifndef CFG_TEXT_PACKETS_3   
#define CFG_TEXT_PACKETS_3    false   // true - enable aprs-lora 3 byte prefix '<', 0xff, 0x01
#endif

// Frequency correction for narrow band bandwidths
#ifndef CFG_FREQ_CORR       
#define CFG_FREQ_CORR         false   // true - correct own frequency based on received packet
#endif
#ifndef CFG_FREQ_CORR_DELTA
#define CFG_FREQ_CORR_DELTA   1000    // correct when frequency difference is larger than this value
#endif

// PTT control
#ifndef CFG_PTT_ENABLE    
#define CFG_PTT_ENABLE        false   // enable external ptt (relay) control (for amplifier)
#endif
#ifndef CFG_PTT_PIN      
#define CFG_PTT_PIN           12      // PTT pin
#endif
#ifndef CFG_PTT_TX_DELAY_MS   
#define CFG_PTT_TX_DELAY_MS   50      // delay between relay switching ON and transmission startup
#endif
#ifndef CFG_PTT_TX_TAIL_MS   
#define CFG_PTT_TX_TAIL_MS    10      // delay between stopping transmission and relay switching OFF
#endif

// enable modem telemetry
#ifndef CFG_TLM_ENABLE
#define CFG_TLM_ENABLE        true   // enable modem battery monitor
#endif
#ifndef CFG_TLM_BAT_MON_CAL
#define CFG_TLM_BAT_MON_CAL   0.0f   // voltage correction, override for specific board type
#endif
#ifndef CFG_TLM_BAT_MON_PIN
#define CFG_TLM_BAT_MON_PIN   36     // battery ADC pin
#endif

#endif // CONFIG_H