#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>

#include "V1_2_SOAR_RTOS_LORA.h"

#define RX 18
#define TX 17

// Globals ----------------------------------------------------------------------------------------
static SemaphoreHandle_t lora_sem;
static SemaphoreHandle_t lora_mutex;
static QueueHandle_t txQu;

// Task handles
TaskHandle_t Task1Handle;
TaskHandle_t lora_transmitHandle;
TaskHandle_t lora_readHandle;

SOAR_Lora lora("6", "5", "915000000"); // LoRa

bool reporting_lock = false;
String lora_input = "";
String address = "";

uint32_t latency_checkpoint = 0;

struct txMessage
{
  char strToSend[32];
  int addressToSend;
};
struct rxMessage
{
  int addressReceived;
  int length;
  byte* strReceived;
  int rssi;
  int snr;
};



// Task1 function
void Task1(void* pvParameters) {
  (void)pvParameters;
  for (;;) {
    // Serial.println("Task 1 is running");
    if (Serial.available() > 0)
    {
      // Serial.println("Read from serial task is executing");
      String userInput = Serial.readStringUntil('\n'); // Read the input until newline character

      // Process user input
      if (userInput.length() > 0)
      {
        int commaIndex = userInput.indexOf(",");
        lora_input = userInput.substring(0, commaIndex);
        address = userInput.substring(commaIndex + 1);
        Serial.println("Received input from serial");

        // char serial_val[100];
        // snprintf(serial_val, sizeof(serial_val), "Lora input: %s, Address: %s", lora_input, address);
        // Serial.println(serial_val);

        txMessage serial_Item;
        strncpy(serial_Item.strToSend, lora_input.c_str(), sizeof(serial_Item.strToSend) - 1);
        serial_Item.addressToSend = address.toInt();
        xQueueSend(txQu, (void*)&serial_Item, portMAX_DELAY);

        // Check if the input ends with ":repeat"
        if (userInput.endsWith(":repeat"))
        {
          // Extract the part before ":"
          int colonIndex = userInput.indexOf(':');
          if (colonIndex != -1)
          {
            String prefix = userInput.substring(0, colonIndex);
            lora_input = prefix;
            // Set reporting_lock to true
            reporting_lock = true;
            // Use 'prefix' variable as needed
          }
        }
        else
        {
          reporting_lock = false;
        }
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}

// lora_transmit function
void lora_transmit(void* pvParameters) {
  (void)pvParameters;
  const uint16_t warningThreshold = 100;

  for (;;) {
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay for 2 seconds

    if (xSemaphoreTake(lora_sem, (500 / portTICK_PERIOD_MS)) == pdTRUE) {
      // Serial.println("Task 2 is running");
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        // Serial.println("Transmit task is executing");
        txMessage item;

        if (xQueueReceive(txQu, (void*)&item, (500 / portTICK_PERIOD_MS)) == pdTRUE) {
          Serial.println("Received item from crit queue");
          lora.stringPacketWTime(item.strToSend, item.addressToSend);
        }
        xSemaphoreGive(lora_mutex);
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);

      xSemaphoreGive(lora_sem);
    }
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    if (highWaterMark < warningThreshold && highWaterMark > 90)
    {
      Serial.print("Warning: lora_transmit is running low on stack memory. High water mark: ");
      Serial.println(highWaterMark);
    }
  }
}

void lora_read(void* pvParameters) {
  (void)pvParameters;
  const uint16_t warningThreshold = 100;
  for (;;) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    if (xSemaphoreTake(lora_sem, (2000 / portTICK_PERIOD_MS)) == pdTRUE) {
      // Serial.println("Task 3 is running");

      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        // Serial.println("Read task is executing");

        rxMessage rxItem;
        byte* data = nullptr;
        bool lora_available = lora.read(&rxItem.addressReceived, &rxItem.length, &data, &rxItem.rssi, &rxItem.snr);

        if (rxItem.length > 0 && lora.checkChecksum(data, rxItem.length)) {
          char response[200];
          snprintf(response, sizeof(response), "Received--- Address: %d, Length: %d, Data: %s, RSSI: %d, SNR: %d",
            rxItem.addressReceived, rxItem.length, data, rxItem.rssi, rxItem.snr);
          Serial.println(response);
          if (latency_checkpoint != 0) {
            Serial.print("Latency: ");
            Serial.println(millis() - latency_checkpoint);
            latency_checkpoint = 0;
          }
          Serial.println("--Packet End--");
        }

        delete[] data; // Clean up

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        xSemaphoreGive(lora_mutex);
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    if (highWaterMark < warningThreshold && highWaterMark > 90)
    {
      Serial.print("Warning: lora_read is running low on stack memory. High water mark: ");
      Serial.println(highWaterMark);
    }

  }
}

void setup() {
  Serial.begin(115200); // Initialize USB Serial
  lora.begin();

  // Create Semaphore
  lora_sem = xSemaphoreCreateBinary();
  if (lora_sem == NULL)
  {
    Serial.println("Could not create lora semaphore");
    while (1)
      ;
  }

  // Create Mutex
  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL)
  {
    Serial.println("Could not create lora mutex");
    while (1)
      ;
  }

  // Create Queue
  txQu = xQueueCreate(2, sizeof(txMessage));
  if (txQu == NULL)
  {
    Serial.println("Could not create tx queue");
    while (1)
      ;
  }

  // Create Tasks
  xTaskCreate(
    Task1,          // Task function
    "Task 1",       // Task name
    512,            // Stack size
    NULL,           // Task input parameter
    1,              // Task priority
    &Task1Handle    // Task handle
  );

  // Create lora_transmit
  xTaskCreate(
    lora_transmit,          // Task function
    "Task 2",       // Task name
    1700,            // Stack size
    NULL,           // Task input parameter
    1,              // Task priority
    &lora_transmitHandle    // Task handle
  );
  xTaskCreate(
    lora_read,
    "Task 3",
    2200,
    NULL,
    1,
    &lora_readHandle
  );

  /* ----------------------------- NOTE ON MEMORY FOR THE ARDUINO MEGA 2560 -----------------------------
  * The Arduino Mega 2560 has 8KB of SRAM. Any memory less than this
  1664
  */

  xSemaphoreGive(lora_sem);
  // Start the scheduler
  vTaskStartScheduler();
}

void loop() {
  // Empty. Things are done in Tasks.
}