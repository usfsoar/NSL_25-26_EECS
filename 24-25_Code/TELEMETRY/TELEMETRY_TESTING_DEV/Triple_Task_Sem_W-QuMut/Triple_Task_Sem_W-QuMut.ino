// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

struct txMessage {
  char strToSend[32];
  int addressToSend;
};
struct rxMessage {
  int addressReceived;
  int length;
  byte *strReceived;
  int rssi;
  int snr;
};

/*

SOAR NSL EECS TELEMETRY 2024-2025
TELEMETRY LEAD: PAVAN MOTURI

METADATA ------------------------------
PURPOSE: Create a set of working code based on RTOS that correctly implements a semaphore that is used by 3 tasks. These are NOT simple tasks.
THIS IS RELATED TO "Triple_Task_Semaphore", BUT IS DIFFERENT. This code is more complex: it implements Queues (Qu) and Mutexes (Mut) in the tasks.
This is developed with the goal of being implemented into the "Command_Receive_Processing" file in "F24_V1_AV_BAY_Pieces". This will ultimately be implemented into the "F24_V1_AV_BAY".

NOTES: A 4th dummy task is needed for 3 tasks to run with the semaphore.
Also, portMAX_DELAY cannot be used in every circumstance. Refer to the following hyperlink for documentation.
https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/system/freertos.html?highlight=xsemaphorecreatemutex#c.xSemaphoreTake


LAST UPDATED: DECEMBER 7TH 2024
CHECK GITHUB FOR VERSION HISTORY
(Committed to the Lora-and-SD-updates branch originally)

*/

// Globals ----------------------------------------------------------------------------------------

// Mutexes --------------------------------------
// LoRa Mutex
static SemaphoreHandle_t lora_mutex;

// Semaphore --------------------------------------
// LoRa Semaphore
static SemaphoreHandle_t lora_sem;
// Mutex Control Semaphore
// static SemaphoreHandle_t mutex_control_sem;

// Task Handles ---------------------------------
TaskHandle_t nonCritTskHndl = NULL;
TaskHandle_t critTskHndl = NULL;
TaskHandle_t rcvTskHndl = NULL;
TaskHandle_t prtclTsksHndl = NULL;

// Queues ---------------------------------------
// Transmit Queues
static QueueHandle_t txNonCritQu;
static QueueHandle_t txCritQu;

// Received Commands Queue
static QueueHandle_t ReceiveQu;


// Tasks ------------------------------------------------------------------------------------------
void lora_transmit_noncrit(void *parameters) {
  while (1) {
    if (xSemaphoreTake(lora_sem, (4000 / portTICK_PERIOD_MS)) == pdTRUE) {
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {

        Serial.println("Transmit NonCrit is executing");

        txMessage item;
        if (xQueueReceive(txNonCritQu, (void *)&item, (4000 / portTICK_PERIOD_MS)) == pdTRUE) {
          Serial.println("Received item from noncrit queue");
        }
        xSemaphoreGive(lora_mutex);
      } else {
        Serial.println("NonCrit Qu failed to take mutex");
      }
      xSemaphoreGive(lora_sem);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
  }
}

void lora_transmit_crit(void *parameters) {
  while (1) {
    if (xSemaphoreTake(lora_sem, (2000 / portTICK_PERIOD_MS)) == pdTRUE) {
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {

        Serial.println("Transmit Crit is executing");
        
        txMessage item;
        if (xQueueReceive(txCritQu, (void *)&item, (2000 / portTICK_PERIOD_MS)) == pdTRUE) {
          Serial.println("Received item from crit queue");
        }
        xSemaphoreGive(lora_mutex);
      } else {
        Serial.println("Crit Qu failed to take mutex");
      }
      xSemaphoreGive(lora_sem);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }
}

void lora_receive(void *parameters) {
  while (1) {
    if (xSemaphoreTake(lora_sem, (1000 / portTICK_PERIOD_MS)) == pdTRUE) {
      if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Receive Task is executing");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreGive(lora_mutex);
      }
      xSemaphoreGive(lora_sem);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Create LoRa Mutex ------------------------------------------------------------------
  lora_mutex = xSemaphoreCreateMutex();
  // Check if Mutex was created successfully
  if (lora_mutex == NULL) {
    Serial.println("Error creating mutex");
    while(1);
  } else {
    Serial.println("Mutex created successfully");
  }
  
  // Create Queues ----------------------------------------------------------------------
  // Transmit Queues
  txNonCritQu = xQueueCreate(5, sizeof(txMessage));
  txCritQu = xQueueCreate(5, sizeof(txMessage));

  // Received Commands Queue
  ReceiveQu = xQueueCreate(5, sizeof(rxMessage));
  // ReceiveQu = xQueueCreate(RECEIVEQU_LEN, 5);

  // Check if Queues were created successfully
  if (txNonCritQu == NULL || txCritQu == NULL || ReceiveQu == NULL) {
    Serial.println("Error creating queues");
    while(1);
  } else {
    Serial.println("Queues created successfully");
  }

  // Create LoRa Semaphore --------------------------------------------------------------
  lora_sem = xSemaphoreCreateBinary();
  // Check if Semaphore was created successfully
  if (lora_sem == NULL) {
    Serial.println("Error creating semaphore");
    while(1);
  } else {
    Serial.println("Lora semaphore created successfully");
  }

  // Tasks -----------------------------------------------------------------------------
  BaseType_t result;

  result = xTaskCreatePinnedToCore(
    lora_transmit_crit,
    "Lora Crit Transmit",
    2048,
    NULL,
    3,
    &critTskHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating lora_transmit_crit task");
    while (1);
  }

  result = xTaskCreatePinnedToCore(
    lora_transmit_noncrit,
    "Lora NonCrit Transmit",
    2048,
    NULL,
    3,
    &nonCritTskHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating lora_transmit_noncrit task");
    while (1);
  }

  result = xTaskCreatePinnedToCore(
    lora_receive,
    "Lora Receive",
    4096,
    NULL,
    3,
    &rcvTskHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating lora_receive task");
    while (1);
  }
  
  xSemaphoreGive(lora_sem);
}


void loop() {
  vTaskDelay(8000 / portTICK_PERIOD_MS);

  txMessage item;
  strncpy(item.strToSend, "WU", sizeof(item.strToSend) - 1);
  item.addressToSend = 6;
  xQueueSend(txCritQu, (void *)&item, portMAX_DELAY);

}




