// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// #include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "V1_SOAR_RTOS_BAROMETER.h"
#include "sensor_data_types.h"

#define SEALEVELPRESSURE_HPA (1013.25)

#include "V1_SOAR_RTOS_LORA.h"

struct txMessage {
  char strToSend[32];
  int addressToSend;
};

// Globals --------------------------------------
SOAR_BAROMETER barometer;

SOAR_Lora lora("7", "5", "915000000", 500);  // LoRa
SemaphoreHandle_t lora_mutex;

static const uint8_t txNonCritQu_len = 5;
static QueueHandle_t txNonCritQu;

static const uint8_t txCritQu_len = 5;
static QueueHandle_t txCritQu;


// Tasks ----------------------------------------
void lora_transmit_noncrit(void *parameters) {
  while(1) {
    txMessage item;
    if (xQueueReceive(txNonCritQu, (void *)&item, portMAX_DELAY) == pdTRUE) {
      // Received item from noncrit queue
      if(lora_mutex != NULL) {
        // Mutex exists noncrit
        if(xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
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

void lora_transmit_crit(void *parameters) {
  while(1) {
    txMessage item;
    if (xQueueReceive(txCritQu, (void *)&item, portMAX_DELAY) == pdTRUE) {
      // Received item from crit queue
      if(lora_mutex != NULL) {
        // Mutex exists
        if(xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
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

void bmpRead(void *parameters) {
  while(1){
    SensorData altimeter_data;
    
    float altimeter = barometer.get_altitude();
    float temp = barometer.get_temperature();
    float pressure = barometer.get_pressure();


    altimeter_data.type = ALTIMETER;
    altimeter_data.data.alt.altitude = altimeter;
    altimeter_data.data.alt.temp = temp;
    altimeter_data.data.alt.pressure = pressure;
  	altimeter_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

    char bmpDataPacket[96];
    snprintf(bmpDataPacket, sizeof(bmpDataPacket), "BMP: %.2fm, %.2fC, %.2fhPa", altimeter, temp, pressure);
    // Max length is 32 chars

    txMessage nonCritNewItem;
    strncpy(nonCritNewItem.strToSend, bmpDataPacket, sizeof(nonCritNewItem.strToSend) - 1);
    nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
    nonCritNewItem.addressToSend = 6;

    // Add to queue
    xQueueSend(txNonCritQu, &nonCritNewItem, portMAX_DELAY);

  }
}


void setup() {
  Serial.begin(115200);
  barometer.Initialize();

  TaskHandle_t nonCritTskHndl = NULL;
  TaskHandle_t critTskHndl = NULL;
 


  // vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---Altimeter_Data_Source Test of the F24 V1 AV Bay Pieces---");

  lora.begin();
  lora.stringPacketWTime("WU",6);

  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL) {
    Serial.println("Error creating mutex");
    while(1);
  }
  txNonCritQu = xQueueCreate(txNonCritQu_len, sizeof(txMessage));
  txCritQu = xQueueCreate(txCritQu_len, sizeof(txMessage));
  
  if (txNonCritQu == NULL || txCritQu == NULL) {
    Serial.println("Error creating queues");
    while(1);
  }

  // Tasks ----------------------------------------
  xTaskCreatePinnedToCore(
    bmpRead,
    "BMP Read TX",
    12288,
    NULL,
    3,
    NULL,
    app_cpu
  );


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


}

void loop() {

}
