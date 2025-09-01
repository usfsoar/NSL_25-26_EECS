// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "V2_SOAR_RTOS_LORA.h"

struct txMessage {
  char strToSend[500];
  int addressToSend;
};
struct rxMessage {
  int addressReceived;
  int length;
  byte* strReceived;
  int rssi;
  int snr;
};

// Globals ----------------------------------------------------------------------------------------
// LoRa -----------------------------------------
SOAR_Lora lora("7", "5", "915000000", 500);

// Delays --------------------------------------
const uint16_t lora_delay = 500;

// Semaphore --------------------------------------
// LoRa Semaphore
static SemaphoreHandle_t lora_sem;

// Task Handles ---------------------------------
TaskHandle_t loratskHndl = NULL;
TaskHandle_t rcvTskHndl = NULL;
TaskHandle_t prtclTsksHndl = NULL;

// Queues ---------------------------------------
// Transmit Queues
static QueueHandle_t loraQu;

// Received Commands Queue
static QueueHandle_t ReceiveQu;


// Tasks ------------------------------------------------------------------------------------------
// LoRa Task -------------------------------
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

        char command[3] = { data[0], data[1], '\0' };
        xQueueSend(ReceiveQu, (void*)&command, portMAX_DELAY);


      }

      delete[] data; // Clean up

      // vTaskDelay(2000 / portTICK_PERIOD_MS);
      vTaskDelay(lora_delay / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}

// Logic Tasks --------------------------------
// Protocol Logic Task
void protocol_logic(void* parameters) {
  while (1) {
    Serial.println("Protocol Logic is executing");
    char command[3];
    if (xQueueReceive(ReceiveQu, (void*)&command, portMAX_DELAY) == pdTRUE) {
      command[sizeof(command) - 1] = '\0'; // Ensure null-termination
      Serial.println("received item from queue receive");
      // Create appropriate filename based on sensor type
      if (strcmp(command, "PI") == 0) {
        Serial.println("Received PI");
        txMessage response;
        strncpy(response.strToSend, "PONG", sizeof(response.strToSend) - 1);
        response.addressToSend = 6;
        xQueueSend(loraQu, (void*)&response, portMAX_DELAY);
        Serial.println("Sent PONG to CRIT Queue");
      }
      else if (strcmp(command, "AO") == 0) {
        Serial.print("Received AO");
      }
      else if (strcmp(command, "AF") == 0) {
        Serial.print("Received AF");
      }
      else if (strcmp(command, "IO") == 0) {
        Serial.print("Received IO");
      }
      else if (strcmp(command, "IF") == 0) {
        Serial.print("Received IF");
      }
      else if (strcmp(command, "DA") == 0) {
        Serial.print("Received DA");
      }
      else if (strcmp(command, "DF") == 0) {
        Serial.print("Received DF");
      }
      else {
        Serial.print("Received command not recognized");
      }
    }
  }
}


void setup() {
  Serial.begin(115200);

  Serial.println("Setup started");
  // Initialize Peripherals -------------------------------------------------------------
  // Initialize and send LoRa message
  lora.begin();

  // Create Queues ----------------------------------------------------------------------
  // Transmit Queues
  loraQu = xQueueCreate(LORAQU_LEN, sizeof(txMessage));

  // Received Commands Queue
  ReceiveQu = xQueueCreate(RECEIVEQU_LEN, sizeof(rxMessage));


  // Check if Queues were created successfully
  if (loraQu == NULL || ReceiveQu == NULL) {
    Serial.println("Error creating queues");
    while (1);
  }
  else {
    Serial.println("Queues created successfully");
  }

  // Create LoRa Semaphore --------------------------------------------------------------
  lora_sem = xSemaphoreCreateBinary();
  // Check if Semaphore was created successfully
  if (lora_sem == NULL) {
    Serial.println("Error creating semaphore");
    while (1);
  }
  else {
    Serial.println("Lora semaphore created successfully");
  }

  // Tasks -----------------------------------------------------------------------------
  BaseType_t result;
  result = xTaskCreatePinnedToCore(
    lora_task,
    "Lora Task",
    8192,
    NULL,
    2,
    &loratskHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating lora task");
    while (1);
  }


  result = xTaskCreatePinnedToCore(
    protocol_logic,
    "Protocol Logic",
    2048,
    NULL,
    1,
    &prtclTsksHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating protocol_logic task");
    while (1);
  }

  Serial.println("setup completed and giving semaphore");

  // Give the semaphores initially to start the tasks
  xSemaphoreGive(lora_sem);

  txMessage wakeup;
  strncpy(wakeup.strToSend, "WU", sizeof(wakeup.strToSend) - 1);
  wakeup.addressToSend = 6;
  xQueueSend(loraQu, (void*)&wakeup, portMAX_DELAY);
}


void loop() {
  txMessage response;
  strncpy(response.strToSend, "123456789123456789123456789123456789123456789", sizeof(response.strToSend) - 1);
  response.addressToSend = 6;
  xQueueSend(loraQu, (void*)&response, portMAX_DELAY);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
}
