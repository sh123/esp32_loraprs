#ifndef VARIANT_H
#define VARIANT_H

// module name
#define USE_SX126X
#define MODULE_NAME           SX1262    // 868/915 MHz modules

#ifndef CFG_LORA_FREQ_RX
#define CFG_LORA_FREQ_RX      868.775e6   // RX frequency in MHz
#endif
#ifndef CFG_LORA_FREQ_TX
#define CFG_LORA_FREQ_TX      868.775e6   // TX frequency in MHz
#endif

// lora pinouts
#define CFG_LORA_PIN_NSS      5
#define CFG_LORA_PIN_RST      27    // could be also 26 on early boards, check schematics
#define CFG_LORA_PIN_DIO1     12    // (sx127x - dio0, sx126x/sx128x - dio1)
#define CFG_LORA_PIN_BUSY     14    // (sx127x - dio1, sx126x/sx128x - busy)
#define CFG_LORA_PIN_RXEN     32    // (sx127x - unused, sx126x - RXEN pin number)
#define CFG_LORA_PIN_TXEN     33    // (sx127x - unused, sx126x - TXEN pin number)

// built in led
#define BUILTIN_LED           2     // heartbeat led

// Enable modem telemetry
#define CFG_TLM_BAT_MON_CAL   0.37f   // voltage correction
#define CFG_TLM_BAT_MON_PIN   36      // battery ADC pin

#endif // VARIANT_H
