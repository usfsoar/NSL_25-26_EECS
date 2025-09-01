#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif


#include "V1_2_SOAR_RTOS_LORA.h"



// Globals ----------------------------------------------------------------------------------------
// Task Handles ---------------------------------
TaskHandle_t loraHandle;
TaskHandle_t serialHandle;

// Task Delays
const uint16_t lora_delay = LORA_DELAY;
const uint16_t serial_read_delay = SERIAL_READ_DELAY;

// LoRa -----------------------------------------
SOAR_Lora lora("6", "5", "915000000", 500);



// Semaphores and Mutexes -----------------------

// LoRa Semaphore
static SemaphoreHandle_t lora_sem;

// LoRa Mutex
static SemaphoreHandle_t lora_mutex;

// Queues ---------------------------------------
static QueueHandle_t loraQu;
static QueueHandle_t serialQu;


struct txMessage
{
  char strToSend[500];
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

// Tasks ------------------------------------------------------------------------------------------
// LoRa Task
void lora_task(void* parameters) {
  while (1) {

    vTaskDelay(lora_delay / portTICK_PERIOD_MS); // Delay for 2 seconds
    if (xSemaphoreTake(lora_sem, (lora_delay / portTICK_PERIOD_MS)) == pdTRUE) {
      txMessage item;

      if (xQueueReceive(loraQu, (void*)&item, (lora_delay / portTICK_PERIOD_MS)) == pdTRUE) {
        Serial.println("Transmitting message from serial queue");
        lora.stringPacketWTime(item.strToSend, item.addressToSend);
      }


      rxMessage rxItem;
      byte* data = nullptr;
      bool lora_available = lora.read(&rxItem.addressReceived, &rxItem.length, &data, &rxItem.rssi, &rxItem.snr);
      Serial.println(lora_available);
      if (rxItem.length > 0 && lora.checkChecksum(data, rxItem.length)) {
        char response[200];
        snprintf(response, sizeof(response), "Received--- Address: %d, Length: %d, Data: %s, RSSI: %d, SNR: %d",
          rxItem.addressReceived, rxItem.length, data, rxItem.rssi, rxItem.snr);
        Serial.println(response);
        Serial.println("----------Packet End----------");
      }

      delete[] data; // Clean up

      // vTaskDelay(2000 / portTICK_PERIOD_MS);
      vTaskDelay(lora_delay / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}

// Serial Task
void serial_read(void* parameters) {
  while (1) {
    if (xSemaphoreTake(lora_sem, (serial_read_delay / portTICK_PERIOD_MS)) == pdTRUE) {
      Serial.println("Serial Read Task is running");

      vTaskDelay(serial_read_delay / portTICK_PERIOD_MS);

      // Read from serial
      if (Serial.available() > 0) {
        String userInput = Serial.readStringUntil('\n');

        if (userInput.length() > 0) {
          Serial.println("Received something from serial");
          int commaIndex = userInput.indexOf(",");

          txMessage txItem;
          strncpy(txItem.strToSend, userInput.substring(0, commaIndex).c_str(), sizeof(txItem.strToSend) - 1);
          txItem.addressToSend = userInput.substring(commaIndex + 1).toInt();
          xQueueSend(loraQu, &txItem, portMAX_DELAY);

        }
      }
      vTaskDelay(serial_read_delay / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize Peripherals -----------------------------------------------------------------------
  lora.begin();

  // Create Queues --------------------------------------------------------------------------------
  // Lora Queue
  loraQu = xQueueCreate(5, sizeof(txMessage));

  // Serial Queue
  serialQu = xQueueCreate(5, sizeof(rxMessage));

  // Check if Queues were created successfully
  if (loraQu == NULL || serialQu == NULL)
  {
    Serial.println("Error creating queues");
    while (1);
  }
  else
  {
    Serial.println("Queues created successfully");
  }

  // Create LoRa Semaphore ------------------------------------------------------------------------
  lora_sem = xSemaphoreCreateBinary();
  if (lora_sem == NULL)
  {
    Serial.println("Could not create lora semaphore");
    while (1);
  }

  // Create LoRa Mutex ----------------------------------------------------------------------------
  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL)
  {
    Serial.println("Could not create lora mutex");
    while (1);
  }

  // Tasks ----------------------------------------------------------------------------------------
  BaseType_t result;

  result = xTaskCreatePinnedToCore(
    lora_task,
    "Lora Task",
    8192,
    NULL,
    2,
    &loraHandle,
    app_cpu
  );
  if (result != pdPASS)
  {
    Serial.println("Error creating lora_task");
    while (1);
  }

  result = xTaskCreatePinnedToCore(
    serial_read,
    "Serial Read",
    2048,
    NULL,
    1,
    &serialHandle,
    app_cpu
  );
  if (result != pdPASS)
  {
    Serial.println("Error creating serial_read");
    while (1);
  }

  xSemaphoreGive(lora_sem);

}

void loop() {

}
