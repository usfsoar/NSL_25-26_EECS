// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

/*

SOAR NSL EECS TELEMETRY 2024-2025
TELEMETRY LEAD: PAVAN MOTURI

METADATA ------------------------------
PURPOSE: Create a set of working code based on RTOS that correctly implements a semaphore that is used by 3 tasks. These are simple tasks.

NOTES: A 4th dummy task is needed for 3 tasks to run with the semaphore.

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

// Task Handles ---------------------------------
TaskHandle_t nonCritTskHndl = NULL;
TaskHandle_t critTskHndl = NULL;
TaskHandle_t rcvTskHndl = NULL;
TaskHandle_t prtclTsksHndl = NULL;


// Tasks ------------------------------------------------------------------------------------------
void lora_transmit_noncrit(void *parameters) {
  while (1) {
    // Serial.println("Transmit NonCrit Task running");
    if(xSemaphoreTake(lora_sem, portMAX_DELAY) == pdTRUE) {
      Serial.println("Transmit NonCrit is executing");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}

void lora_transmit_crit(void *parameters) {
  while (1) {
    // Serial.println("Transmit Crit Task running");
    if(xSemaphoreTake(lora_sem, portMAX_DELAY) == pdTRUE) {
      Serial.println("Transmit Crit is executing");
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}

void lora_receive(void *parameters) {
  while (1) {
    // Serial.println("Receive Task running");
    if(xSemaphoreTake(lora_sem, portMAX_DELAY) == pdTRUE) {
      Serial.println("Receive Task is executing");
      vTaskDelay(6000 / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
    xSemaphoreGive(lora_sem);
  }
}

void lora_dummy(void *paramateres) {
  while (1) {
    xSemaphoreGive(lora_sem);
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
  
  // Create LoRa Semaphore --------------------------------------------------------------
  lora_sem = xSemaphoreCreateBinary();
  // Check if Semaphore was created successfully
  if (lora_sem == NULL) {
    Serial.println("Error creating semaphore");
    while(1);
  } else {
    Serial.println("Semaphore created successfully");
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
  result = xTaskCreatePinnedToCore(
    lora_dummy,
    "Lora Dummy",
    1024,
    NULL,
    3,
    NULL,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating lora_dummy task");
    while (1);
  }

  // Lora dummy task is needed or else the crit task will only run over and over again
  // xSemaphoreGive(lora_sem);

}


void loop() {

}




