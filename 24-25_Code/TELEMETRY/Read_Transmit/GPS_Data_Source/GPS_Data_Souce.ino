/*
 * GPS_Data_Source.ino
 * 
 * This program collects GPS data from an ESP32 device using the SOAR_GPS class and transmits it over LoRa.
 * It uses FreeRTOS to handle non-critical and critical data separately, utilizing queues and tasks.
 * 
 * Libraries Used:
 * - SOAR_GPS.h: Custom class for handling GPS communication and parsing.
 * - SOAR_RTOS_LORA.h: Provides LoRa transmission functions for critical and non-critical messages.
 * 
 * Hardware Setup:
 * - RX_PIN: The RX pin on the ESP32 where the GPS TX is connected.
 * - TX_PIN: The TX pin on the ESP32 where the GPS RX is connected.
 * 
 * Functions:
 * - setup(): Initializes Serial for debugging, GPS setup, and LoRa transmission queues and tasks.
 * - loop(): Empty since tasks handle all operations.
 * - gpsRead(): Reads GPS data and sends non-critical messages to LoRa.
 * - lora_transmit_noncrit(): Transmits non-critical GPS data over LoRa.
 * - lora_transmit_crit(): Transmits critical GPS data over LoRa.
 */

#include <SOAR_GPS.h>
#include "SOAR_RTOS_LORA.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pin definitions for GPS
#define RX_PIN 16
#define TX_PIN 17
#define SERIAL_BUS 1 // ESP32 Serial bus (e.g., Serial1)

struct txMessage {
  char strToSend[32];
  int addressToSend;
};

// Globals --------------------------------------
SOAR_GPS gps(SERIAL_BUS, RX_PIN, TX_PIN); // GPS
SOAR_Lora lora("7", "5", "915000000", 500); // LoRa
static SemaphoreHandle_t lora_mutex;

static const uint8_t txNonCritQu_len = 5;
static QueueHandle_t txNonCritQu;

static const uint8_t txCritQu_len = 5;
static QueueHandle_t txCritQu;

// Tasks ----------------------------------------

void gpsRead(void *parameters) {
  char nmea[100];
  bool ready = false;
  bool failed = false;

  while (1) {
    gps.GET_NMEA(nmea, &ready, &failed);

    if (ready) {
      char gpsDataPacket[32];
      snprintf(gpsDataPacket, sizeof(gpsDataPacket), "GPS: %s", nmea);

      // Prepare non-critical message for LoRa transmission
      txMessage nonCritNewItem;
      strncpy(nonCritNewItem.strToSend, gpsDataPacket, sizeof(nonCritNewItem.strToSend) - 1);
      nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensure null-termination
      nonCritNewItem.addressToSend = 6;

      // Add to the non-critical queue
      xQueueSend(txNonCritQu, &nonCritNewItem, portMAX_DELAY);
    } else if (failed) {
      Serial.println("Failed to parse GPS data.");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay between readings
  }
}

void lora_transmit_noncrit(void *parameters) {
  txMessage item;
  while (1) {
    if (xQueueReceive(txNonCritQu, (void *)&item, portMAX_DELAY) == pdTRUE) {
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        lora.stringPacketWTime(item.strToSend, item.addressToSend);
        xSemaphoreGive(lora_mutex);
      }
    }
  }
}

void lora_transmit_crit(void *parameters) {
  txMessage item;
  while (1) {
    if (xQueueReceive(txCritQu, (void *)&item, portMAX_DELAY) == pdTRUE) {
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        lora.beginPacket();
        lora.stringPacketWTime(item.strToSend, item.addressToSend);
        xSemaphoreGive(lora_mutex);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  gps.setup();

  Serial.println();
  Serial.println("---GPS LoRa Transmit---");

  lora.begin();

  // Initialize mutex and queues
  lora_mutex = xSemaphoreCreateMutex();
  txNonCritQu = xQueueCreate(txNonCritQu_len, sizeof(txMessage));
  txCritQu = xQueueCreate(txCritQu_len, sizeof(txMessage));

  // Create tasks
  xTaskCreatePinnedToCore(
    gpsRead,
    "GPS Read TX",
    2048,
    NULL,
    3,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    lora_transmit_noncrit,
    "LoRa NonCrit Transmit",
    2048,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    lora_transmit_crit,
    "LoRa Crit Transmit",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );
}

void loop() {
  // Empty loop as FreeRTOS tasks handle the functionality
}
