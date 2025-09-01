// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "V1_3_SOAR_RTOS_LORA.h"

/*
Part of the F24 V1 AV Bay Pieces, this code is meant to test the LORA_Transmit functionality of the F24 V1 AV Bay Pieces.
This code is based on Espressif FreeRTOS and runs on the ESP32-S3 microcontroller. It utilizes two queues to send data to
two different tasks, the noncrit and crit tasks. The noncrit task has a lower priority than the crit task. This ensures
the noncrit task runs only when the crit task is not running. A mutex is used to ensure that only one task can access the
LoRa module at a time. In this code, the messages are sent to the queue in the main loop, but the code as a whole is standalone.
*/

// Defines a format for the messages to be sent to the LoRa module
struct txMessage {
  char strToSend[500];
  int addressToSend;
};

// Globals --------------------------------------
// Initializes LoRa, then defines mutex, and then defines two queues
SOAR_Lora lora("7", "5", "915000000", 500);
SemaphoreHandle_t lora_mutex;

static const uint8_t txNonCritQu_len = 5;
static QueueHandle_t txNonCritQu;

static const uint8_t txCritQu_len = 5;
static QueueHandle_t txCritQu;

// Tasks ----------------------------------------
// Defines noncrit task, then crit task
void lora_transmit_noncrit(void* parameters) {
  while (1) {
    txMessage item;
    if (xQueueReceive(txNonCritQu, (void*)&item, portMAX_DELAY) == pdTRUE) {
      // Received item from noncrit queue
      if (lora_mutex != NULL) {
        // Mutex exists noncrit
        if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
          // Mutex taken, beginning transmission
          // int address, length, rssi, snr;
          // byte *data;

          // Send command
          Serial.print("String to Send: ");
          Serial.println(item.strToSend);
          lora.stringPacketWTime(item.strToSend, item.addressToSend);

          // Return mutex
          xSemaphoreGive(lora_mutex);
        }
      }
    }
  }
}

void lora_transmit_crit(void* parameters) {
  while (1) {
    txMessage item;
    if (xQueueReceive(txCritQu, (void*)&item, portMAX_DELAY) == pdTRUE) {
      // Received item from crit queue
      if (lora_mutex != NULL) {
        // Mutex exists
        if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
          // Mutex taken, beginning transmission

          // int address, length, rssi, snr;
          // byte *data;
          // Send command
          lora.stringPacketWTime(item.strToSend, item.addressToSend);
          // Return mutex
          xSemaphoreGive(lora_mutex);
        }
      }
    }
  }
}


void setup() {
  Serial.begin(115200);

  // Handles to tasks
  TaskHandle_t nonCritTskHndl = NULL;
  TaskHandle_t critTskHndl = NULL;

  // Arbitrary delay to make sure the serial monitor is open
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---LORA_Transmit Test of the F24 V1 AV Bay Pieces---");

  // Begin LoRa and send a wakeup message
  lora.begin();
  lora.stringPacketWTime("WU", 6);

  // Mutex and Queues ------------------------------
  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL) {
    Serial.println("Error creating mutex");
    while (1);
  }
  txNonCritQu = xQueueCreate(txNonCritQu_len, sizeof(txMessage));
  txCritQu = xQueueCreate(txCritQu_len, sizeof(txMessage));

  if (txNonCritQu == NULL || txCritQu == NULL) {
    Serial.println("Error creating queues");
    while (1);
  }

  // Tasks ----------------------------------------
  xTaskCreatePinnedToCore(
    lora_transmit_noncrit,
    "Lora NonCrit Transmit",
    2048,
    NULL,
    2,
    &nonCritTskHndl,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    lora_transmit_crit,
    "Lora Crit Transmit",
    2048,
    NULL,
    1,
    &critTskHndl,
    app_cpu
  );

  // vTaskDelete(NULL);
}

void loop() {
  // Add to txNonCritQu ---------------------------------------------------------------------------
  txMessage nonCritNewItem;
  strncpy(nonCritNewItem.strToSend, "012345678901234567890123456789012345678900123456789", sizeof(nonCritNewItem.strToSend) - 1);
  nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
  nonCritNewItem.addressToSend = 6;
  // Received: 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
  // Transmitted: 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789

    // Add to queue
  xQueueSend(txNonCritQu, &nonCritNewItem, portMAX_DELAY);

  // Add to txCritQu ------------------------------------------------------------------------------

  // NOTE -----------------------------
  // This is an arbitrary delay of 5 seconds to simulate the crit task running less frequently
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  // ----------------------------------

  txMessage CritNewItem;
  strncpy(CritNewItem.strToSend, "Crit Qu Message", sizeof(CritNewItem.strToSend) - 1);
  CritNewItem.strToSend[sizeof(CritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
  CritNewItem.addressToSend = 6;

  xQueueSend(txCritQu, &CritNewItem, portMAX_DELAY);

}
