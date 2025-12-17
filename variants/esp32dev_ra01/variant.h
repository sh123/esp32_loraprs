#ifndef VARIANT_H
#define VARIANT_H

// module name
#define MODULE_NAME           SX1278

// module pinouts
#define CFG_LORA_PIN_SS       5
#define CFG_LORA_PIN_RST      26
#define CFG_LORA_PIN_A        12
#define CFG_LORA_PIN_B        RADIOLIB_NC
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
#define CFG_TLM_BAT_MON_CAL   0.37f   // calibration coefficient
#define CFG_TLM_BAT_MON_PIN   36      // battery ADC pin

#endif // VARIANT_H
