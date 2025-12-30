#ifndef VARIANT_H
#define VARIANT_H

// lora module
#define USE_SX126X
#define MODULE_NAME           SX1262
//#define MODULE_NAME           SX1268

// lora pinouts
#define CFG_LORA_PIN_SS       7
#define CFG_LORA_PIN_RST      8
#define CFG_LORA_PIN_A        3
#define CFG_LORA_PIN_B        10
#define CFG_LORA_PIN_RXEN     21
#define CFG_LORA_PIN_TXEN     20

// not in use
#ifdef BUILTIN_LED
#undef BUILTIN_LED 
#endif

// Bluetooth
#define CFG_BT_NAME           "loraprs"
#define CFG_BT_USE_BLE        true

// Enable modem telemetry
#define CFG_TLM_ENABLE        true   // enable modem battery monitor
#define CFG_TLM_BAT_MON_CAL   0.37f   // calibration coefficient
#define CFG_TLM_BAT_MON_PIN   0      // battery ADC pin

#endif // VARIANT_H