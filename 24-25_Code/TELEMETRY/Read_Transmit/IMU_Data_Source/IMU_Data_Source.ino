#include "V2_1_SOAR_RTOS_IMU.h"
#include "V1_SOAR_RTOS_LORA.h"
#include "sensor_data_types.h"
#include "SPI.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define DEBUG_IMU false
#define BNO_RESET_PIN 1

struct txMessage {
  char strToSend[500];
  int addressToSend;
};

// Globals --------------------------------------
SOAR_IMU imu_sensor;
SOAR_Lora lora("7", "5", "915000000", 500);  // LoRa
SemaphoreHandle_t lora_mutex;

static const uint8_t txNonCritQu_len = 5;
static QueueHandle_t txNonCritQu;

static const uint8_t txCritQu_len = 5;
static QueueHandle_t txCritQu;

SemaphoreHandle_t bno_mutex;

// Tasks ----------------------------------------
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

void bnoRead(void* parameters) {
  while (1) {
    if (xSemaphoreTake(bno_mutex, portMAX_DELAY) == pdTRUE) {


      SensorData imu_data;
      Serial.println("Beginning BNO Read");
      imu_sensor.loop_iterations++;

      // BNO: Initializing variables
      float accel[3];
      float lin_accel[3];
      float gravity[3];
      float quat[4];
      float gyro[3];

      // BNO: Getting data values
      imu_sensor.GET_ACCELERATION(accel);
      imu_sensor.GET_LINEARACCEL(lin_accel);
      imu_sensor.GET_GRAVITY(gravity);
      imu_sensor.GET_QUAT(quat);
      imu_sensor.GET_GYROSCOPE(gyro);

      // Filling out struct to send to data handling task
      imu_data.type = IMU;
      imu_data.data.imu.accel[0] = accel[0];
      imu_data.data.imu.accel[1] = accel[1];
      imu_data.data.imu.accel[2] = accel[2];
      imu_data.data.imu.linear[0] = lin_accel[0];
      imu_data.data.imu.linear[1] = lin_accel[1];
      imu_data.data.imu.linear[2] = lin_accel[2];
      imu_data.data.imu.gravity[0] = gravity[0];
      imu_data.data.imu.gravity[1] = gravity[1];
      imu_data.data.imu.gravity[2] = gravity[2];
      imu_data.data.imu.quat[0] = quat[0];
      imu_data.data.imu.quat[1] = quat[1];
      imu_data.data.imu.quat[2] = quat[2];
      imu_data.data.imu.quat[3] = quat[3];
      imu_data.data.imu.gyro[0] = gyro[0];
      imu_data.data.imu.gyro[1] = gyro[1];
      imu_data.data.imu.gyro[2] = gyro[2];
      imu_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

      // Create bnoDataPacket
      char bnoDataPacket[500];

      // Print to console
      snprintf(bnoDataPacket, sizeof(bnoDataPacket), "BNO: [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f,A%.2f], [X%.2f,Y%.2f,Z%.2f]", accel[0], accel[1], accel[2], lin_accel[0], lin_accel[1], lin_accel[2], gravity[0], gravity[1], gravity[2], quat[0], quat[1], quat[2], quat[3], gyro[0], gyro[1], gyro[2]);
      // snprintf(bnoDataPacket, sizeof(bnoDataPacket), "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"); 
      // Max length of IMU data is 149 chars

      // Send directly to the tx noncrit queue
      txMessage nonCritNewItem;
      strncpy(nonCritNewItem.strToSend, bnoDataPacket, sizeof(nonCritNewItem.strToSend) - 1);
      nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
      nonCritNewItem.addressToSend = 6;

      xQueueSend(txNonCritQu, &nonCritNewItem, portMAX_DELAY);
      xSemaphoreGive(bno_mutex);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BNO_RESET_PIN, OUTPUT);
  digitalWrite(BNO_RESET_PIN, HIGH);
  imu_sensor.BNO_SETUP();



  TaskHandle_t nonCritTskHndl = NULL;
  TaskHandle_t critTskHndl = NULL;


  bno_mutex = xSemaphoreCreateMutex();
  if (bno_mutex == NULL) {
    Serial.println("Error creating mutex");
    while (1);
  }


  // vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---IMU_Data_Source Test of the F24 V1 AV Bay Pieces---");

  lora.begin();
  lora.stringPacketWTime("WU", 6);

  lora_mutex = xSemaphoreCreateMutex();
  if (lora_mutex == NULL) {
    Serial.println("Error creating mutex");
    while (1);
  }
  txNonCritQu = xQueueCreate(txNonCritQu_len, 500);
  txCritQu = xQueueCreate(txCritQu_len, 500);

  if (txNonCritQu == NULL || txCritQu == NULL) {
    Serial.println("Error creating queues");
    while (1);
  }
  // Tasks ----------------------------------------
  xTaskCreatePinnedToCore(
    bnoRead,
    "BNO Read TX",
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
//   void SOAR_IMU::BNO_SETUP() {

//   Serial.println("Orientation Sensor Test"); Serial.println("");

//   /* Initialise the sensor */
//   if(!this->bno.begin())
//   {
//     /* There was a problem detecting the BNO055 ... check your connections */
//     Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
//   }

//   delay(1000);
//   this->bno.setExtCrystalUse(true);
//   Serial.print("Successful\n");
// }

//   void transmit_IMU_Data(void *pvParameters){
//     float *SOAR_IMU::GET_ACCELERATION() {}
//     float *SOAR_IMU::GET_LINEARACCEL() {}
//     float *SOAR_IMU::GET_GRAVITY() {}
//     float *SOAR_IMU::GET_GYROSCOPE() {}
//     float *SOAR_IMU::GET_QUAT() {}
//     float *SOAR_IMU::GET_VELOCITY() {}
// }



