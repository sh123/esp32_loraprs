#ifndef VARIANT_H
#define VARIANT_H

// module name
#define MODULE_NAME           SX1278

// module pinouts
#define CFG_LORA_PIN_NSS      5
#define CFG_LORA_PIN_RST      27    // could be also 26 on early boards, check schematics
#define CFG_LORA_PIN_DIO1     12    // DIO0 on RA1 module
#define CFG_LORA_PIN_BUSY     RADIOLIB_NC
#define CFG_LORA_PIN_RXEN     RADIOLIB_NC
#define CFG_LORA_PIN_TXEN     RADIOLIB_NC

// built-in led
#ifndef BUILTIN_LED
#define BUILTIN_LED           2
#endif

// Bluetooth
#define CFG_BT_NAME           "loraprs"
#define CFG_BT_USE_BLE        false

// Enable modem telemetry
#define CFG_TLM_ENABLE        true   // enable modem battery monitor
#define CFG_TLM_BAT_MON_CAL   0.37f  // voltage correction
#define CFG_TLM_BAT_MON_PIN   36     // battery ADC pin

#endif // VARIANT_H
