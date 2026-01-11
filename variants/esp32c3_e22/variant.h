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

// led is not in use
#ifdef BUILTIN_LED
#undef BUILTIN_LED
#endif

// modem telemetry
#define CFG_TLM_BAT_MON_CAL   (-0.45f)  // voltage correction
#define CFG_TLM_BAT_MON_PIN   0      // battery ADC pin

#endif // VARIANT_H
