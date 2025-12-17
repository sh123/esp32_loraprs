#ifndef VARIANT_H
#define VARIANT_H

// module name
#define USE_SX126X
#define MODULE_NAME           SX1268

// lora pinouts
#define CFG_LORA_PIN_SS       5
#define CFG_LORA_PIN_RST      26
#define CFG_LORA_PIN_A        12    // (sx127x - dio0, sx126x/sx128x - dio1)
#define CFG_LORA_PIN_B        14    // (sx127x - dio1, sx126x/sx128x - busy)
#define CFG_LORA_PIN_RXEN     32    // (sx127x - unused, sx126x - RXEN pin number)
#define CFG_LORA_PIN_TXEN     33    // (sx127x - unused, sx126x - TXEN pin number)

// built in led
#ifndef BUILTIN_LED
#define BUILTIN_LED           2
#endif

// Bluetooth
#define CFG_BT_NAME           "loraprs"
#define CFG_BT_USE_BLE        false

// Enable modem telemetry
#define CFG_TLM_ENABLE        true    // enable modem battery monitor
#define CFG_TLM_BAT_MON_CAL   0.37f   // calibration coefficient
#define CFG_TLM_BAT_MON_PIN   36      // battery ADC pin

#endif // VARIANT_H