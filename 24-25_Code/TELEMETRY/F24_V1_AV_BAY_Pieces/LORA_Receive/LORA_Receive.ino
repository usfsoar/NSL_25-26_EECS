// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include "V1_2_SOAR_RTOS_LORA.h"

struct txMessage {
  char strToSend[32];
  int addressToSend;
};


// Globals --------------------------------------
SOAR_Lora lora("7", "5", "915000000", 500);  // LoRa
SemaphoreHandle_t lora_mutex;
// static TimerHandle_t lora_receive_timer = NULL;


// Callbacks ------------------------------------


// Tasks ----------------------------------------
void lora_receive(void *parameters) {
  while(1) {
    Serial.println("Processing receive task");
    if(lora_mutex != NULL) {
      Serial.println("Mutex exists receive");
      if(xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Mutex taken, receiving");
        int address, length, rssi, snr;
        byte *data;

        bool lora_available = lora.read(&address, &length, &data, &rssi, &snr);
        
        if (lora_available && length > 0 && lora.checkChecksum(data, length)) {
          Serial.println("Command received, beginning processing");
          char command[3] = {data[0], data[1], '\0'};
          char response[60];
          snprintf(response, sizeof(response), "Received command: %s,RSSI:%d,SNR:%d,FROM:%d", command, rssi, snr, address);
          // Max size of this string is 52 chars => make the response variable to be 60 chars for safety
          // This means define the size of the largest item that ReceiveQu will ever receive to be 60
          // This is why when defining ReceiveQu in the setup function, 60 is given as the size
          Serial.println(response);
          // xQueueSend(ReceiveQu, (void *)&command, portMAX_DELAY);
        }
        
        xSemaphoreGive(lora_mutex);
        Serial.println("Mutex returned");
      }
    }
    // const TickType_t rcvTskDelayVal = 1000 / portTICK_PERIOD_MS;
    vTaskDelay(pdMS_TO_TICKS(RCV_TSK_DELAY));
  }
}



// void lora_timer_callback(TimerHandle_t lora_receive_timer) {
//   if ((uint32_t)pvTimerGetTimerID(lora_receive_timer) != 0) {
//     Serial.println("Timer callback executed");
//     vTaskResume(rcvTskHndl);
//   }
// }



void setup() {
  Serial.begin(115200);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---LORA_Receive Test of the F24 V1 AV Bay Pieces---");

  lora.begin();
  lora.stringPacketWTime("WU",6);

  TaskHandle_t rcvTskHndl = NULL;

  // Timer Setup --------------------------------
  // lora_receive_timer = xTimerCreate(
  //   "Lora Receive Timer", 
  //   1000 / portTICK_PERIOD_MS, 
  //   pdTRUE, 
  //   (void *)0, 
  //   lora_receive);
  

  // if (lora_receive_timer == NULL) {
  //   Serial.println("Error creating timer");
  //   while(1);
  // } else{
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  //   Serial.println("Starting timer");

  //   xTimerStart(lora_receive_timer, 0);
  // }

  // Mutex Setup --------------------------------
  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL) {
    Serial.println("Error creating mutex");
    while(1);
  }

  // Tasks --------------------------------------
  xTaskCreatePinnedToCore(
    lora_receive,
    "Lora Receive",
    2048,
    NULL,
    1,
    &rcvTskHndl,
    app_cpu
  );

  // vTaskSuspend(rcvTskHndl);

}

void loop() {
  // put your main code here, to run repeatedly:

}
