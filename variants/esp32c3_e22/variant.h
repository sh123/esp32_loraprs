#ifndef VARIANT_H
#define VARIANT_H

// lora module
#define USE_SX126X
//#define MODULE_NAME           SX1262    // 868/915 MHz modules
#define MODULE_NAME           SX1268    // 433 MHz modules

// lora pinouts
#define CFG_LORA_PIN_NSS      7
#define CFG_LORA_PIN_RST      8
#define CFG_LORA_PIN_DIO1     3
#define CFG_LORA_PIN_BUSY     10
#define CFG_LORA_PIN_RXEN     21
#define CFG_LORA_PIN_TXEN     20

// not in use
#undef BUILTIN_LED 

// bluetooth
#define CFG_BT_USE_BLE        true

// enable modem telemetry
#define CFG_TLM_ENABLE        true   // enable modem battery monitor
#define CFG_TLM_BAT_MON_CAL   0.37f   // calibration coefficient
#define CFG_TLM_BAT_MON_PIN   0      // battery ADC pin

#endif // VARIANT_H
