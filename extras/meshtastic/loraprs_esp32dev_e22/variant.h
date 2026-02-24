#ifndef _VARIANT_LORAPRS_ESP32DEV_E22_
#define _VARIANT_LORAPRS_ESP32DEV_E22_
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// LoRa
#define USE_SX1262
#define USE_SX1268

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET (27)
#define LORA_DIO1 (12)
#define LORA_RXEN (32)
#define LORA_TXEN (33)
#define LORA_BUSY (14)
#define LORA_SCK (18)
#define LORA_MISO (19)
#define LORA_MOSI (23)
#define LORA_CS (5)

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_RXEN LORA_RXEN
#define SX126X_TXEN LORA_TXEN

#define SX126X_DIO3_TCXO_VOLTAGE (1.8)
#define TCXO_OPTIONAL // make it so that the firmware can try both TCXO and XTAL

#ifdef __cplusplus
}
#endif

#endif // _VARIANT_LORAPRS_ESP32C3_E22_
