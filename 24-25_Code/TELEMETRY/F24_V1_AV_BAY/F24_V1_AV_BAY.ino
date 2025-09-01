/*
-------------------------------------------------------------------------------
UNIVERSITY OF SOUTH FLORIDA
SOCIETY OF AERONAUTICS AND ROCKETRY
NASA STUDENT LAUNCH UNIVERSITY STUDENT LAUNCH INITIATIVE 2024-2025

EECS TELEMETRY AVIONICS BAY
FALL 2024 (F24) AVIONICS BAY V1

Inaugaral launch of this bay: Saturday December 21st, 2024
-------------------------------------------------------------------------------
*/

/*Idea to save for later: make definitions in the config file for the various delays and
use TickType_t to define variables. This allows easy quick config of delays thorugh config file*/

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "V1_SOAR_RTOS_BAROMETER.h"
#include "V2_SOAR_RTOS_LORA.h"
#include "V2_1_SOAR_RTOS_IMU.h"
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "sensor_data_types.h"
// #include "V2_SOAR_SPEAKER.h"
#include "_config.h"

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
const char* LORA_FILEPATH = "/lora.csv";
const char* ALTIMETER_FILEPATH = "/altimeter.csv";
const char* IMU_FILEPATH = "/imu.csv";




// Sensors --------------------------------------
SOAR_BAROMETER barometer;
SOAR_IMU imu_sensor;

// Outputs --------------------------------------
// SOAR_Speaker speaker;
SOAR_SD_CARD sd_card(D0);

// LoRa -----------------------------------------
SOAR_Lora lora("7", "5", "915000000", 500);

// Delays --------------------------------------
const uint16_t lora_delay = LORA_DELAY;
const uint16_t lora_read_delay = LORA_READ_DELAY;
const uint16_t bmp_delay = BMP_DELAY;
const uint16_t bno_delay = BNO_DELAY;
const uint16_t data_handling_delay = DATA_HANDLING_DELAY;

// Semaphore --------------------------------------
// LoRa Semaphore
static SemaphoreHandle_t lora_sem;

// Sensor Semaphore
static SemaphoreHandle_t sensor_sem;

// BNO Mutex
SemaphoreHandle_t bno_mutex;

// Queues ---------------------------------------
// Lora Queue
static QueueHandle_t loraQu;

// SD Card Queue
static QueueHandle_t sdStoreQu;

// Received Commands Queue
static QueueHandle_t ReceiveQu;

// Data Handling Queue
static QueueHandle_t Data_HndlQu;
static float init_altitude = 0;

// Task Handles ---------------------------------
TaskHandle_t loratskHndl = NULL;
TaskHandle_t bmpTskHndl = NULL;
TaskHandle_t bnoTskHndl = NULL;
TaskHandle_t rcvTskHndl = NULL;
TaskHandle_t sdTskHndl = NULL;
TaskHandle_t prtclTsksHndl = NULL;
TaskHandle_t DtaTskHndl = NULL;

// Queue Lengths are defined in config.h and is used when defining the queue

// Tasks ------------------------------------------------------------------------------------------
// LoRa Task -------------------------------
void lora_task(void* parameters) {
  while (1) {

    vTaskDelay(lora_delay / portTICK_PERIOD_MS); // Delay for 2 seconds
    if (xSemaphoreTake(lora_sem, (lora_delay / portTICK_PERIOD_MS)) == pdTRUE) {
      txMessage item;

      if (xQueueReceive(loraQu, (void*)&item, (lora_delay / portTICK_PERIOD_MS)) == pdTRUE) {
        // Serial.println("Transmitting message from serial queue");
        lora.stringPacketWTime(item.strToSend, item.addressToSend);
      }


      rxMessage rxItem;
      byte* data = nullptr;
      bool lora_available = lora.read(&rxItem.addressReceived, &rxItem.length, &data, &rxItem.rssi, &rxItem.snr);
      // Serial.println(lora_available);
      if (rxItem.length > 0 && lora.checkChecksum(data, rxItem.length)) {
        char response[200];
        snprintf(response, sizeof(response), "Received--- Address: %d, Length: %d, Data: %s, RSSI: %d, SNR: %d",
          rxItem.addressReceived, rxItem.length, data, rxItem.rssi, rxItem.snr);
        Serial.println(response);
        Serial.println("----------Packet End----------");

        char command[3] = { data[0], data[1], '\0' };
        xQueueSend(ReceiveQu, (void*)&command, portMAX_DELAY);

        SensorData lora_data;
        lora_data.type = LORA;
        snprintf(lora_data.data.lora.lora_command, sizeof(lora_data.data.lora.lora_command), "%s", command);
        lora_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);
        xQueueSend(sdStoreQu, &lora_data, portMAX_DELAY);

      }

      delete[] data; // Clean up

      // vTaskDelay(2000 / portTICK_PERIOD_MS);
      vTaskDelay(lora_delay / portTICK_PERIOD_MS);
      xSemaphoreGive(lora_sem);
    }
  }
}


// Sensor Tasks --------------------------------
// Altimeter Task
void bmpRead(void* parameters) {
  const uint16_t warningThreshold = 100;
  TickType_t start_tick, end_tick;

  while (1) {
    vTaskDelay(bmp_delay / portTICK_PERIOD_MS);
    if (xSemaphoreTake(sensor_sem, (bmp_delay / portTICK_PERIOD_MS)) == pdTRUE)
    {
      start_tick = xTaskGetTickCount();
      // BMP code

      SensorData altimeter_data;

      float altimeter = barometer.get_altitude();
      float temp = barometer.get_temperature();
      float pressure = barometer.get_pressure();

      altimeter_data.type = ALTIMETER;
      altimeter_data.data.alt.altitude = altimeter;
      altimeter_data.data.alt.temp = temp;
      altimeter_data.data.alt.pressure = pressure;
      altimeter_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

      // Send to the data handling queue directly
      xQueueSend(Data_HndlQu, &altimeter_data, portMAX_DELAY);

      // -------------------- Use this code to send directly to tx noncrit queue ----------------------------------------
      // char bmpDataPacket[96];
      // snprintf(bmpDataPacket, sizeof(bmpDataPacket), "BMP: %.2fm, %.2fC, %.2fhPa", altimeter, temp, pressure);
      // // Max length is 32 chars

      // txMessage nonCritNewItem;
      // strncpy(nonCritNewItem.strToSend, bmpDataPacket, sizeof(nonCritNewItem.strToSend) - 1);
      // nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
      // nonCritNewItem.addressToSend = 6;

      // xQueueSend(loraQu, &nonCritNewItem, portMAX_DELAY);
      // ----------------------------------------------------------------------------------------------------------------

      // Semaphore handling
      end_tick = xTaskGetTickCount();
      xSemaphoreGive(sensor_sem);
      vTaskDelay(bmp_delay / portTICK_PERIOD_MS);
    }
    char ticks[100];
    if (end_tick != 0)
    {
      // snprintf(ticks, sizeof(ticks), "Time taken for bmpRead: %d", (end_tick - start_tick) / portTICK_PERIOD_MS);
      Serial.println(ticks);
    }

    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    if (highWaterMark < warningThreshold)
    {
      Serial.print("Warning: bmpRead task is running low on stack memory. High water mark: ");
      Serial.println(highWaterMark);
    }
  }
}

// BNO Task
void bnoRead(void* parameters) {
  const uint16_t warningThreshold = 100;
  TickType_t start_tick, end_tick;

  while (1) {
    vTaskDelay(bno_delay / portTICK_PERIOD_MS);
    if (xSemaphoreTake(sensor_sem, (bno_delay / portTICK_PERIOD_MS)) == pdTRUE) {
      if (xSemaphoreTake(bno_mutex, portMAX_DELAY) == pdTRUE) {
        start_tick = xTaskGetTickCount();
        // BNO Code

        SensorData imu_data;
        // Serial.println("Beginning BNO Read");
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

        // Send to the data handling queue directly
        xQueueSend(Data_HndlQu, &imu_data, portMAX_DELAY);

        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
        if (highWaterMark < warningThreshold)
        {
          Serial.print("Warning: bnoRead task is running low on stack memory. High water mark: ");
          Serial.println(highWaterMark);
        }

        // -------------------- Use this code to send directly to tx noncrit queue ----------------------------------------
        // Create bnoDataPacket
        // char bnoDataPacket[500];

        // Print to console
        // snprintf(bnoDataPacket, sizeof(bnoDataPacket), "BNO: [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f], [X%.2f,Y%.2f,Z%.2f,A%.2f], [X%.2f,Y%.2f,Z%.2f]", accel[0], accel[1], accel[2], lin_accel[0], lin_accel[1], lin_accel[2], gravity[0], gravity[1], gravity[2], quat[0], quat[1], quat[2], quat[3], gyro[0], gyro[1], gyro[2]);
        // snprintf(bnoDataPacket, sizeof(bnoDataPacket), "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
        // Max length of IMU data is 149 chars

        // Send directly to the tx noncrit queue
        // txMessage nonCritNewItem;
        // strncpy(nonCritNewItem.strToSend, bnoDataPacket, sizeof(nonCritNewItem.strToSend) - 1);
        // nonCritNewItem.strToSend[sizeof(nonCritNewItem.strToSend) - 1] = '\0';  // Ensures null-termination
        // nonCritNewItem.addressToSend = 6;
        // xQueueSend(loraQu, &nonCritNewItem, portMAX_DELAY);
        // ----------------------------------------------------------------------------------------------------------------
        xSemaphoreGive(bno_mutex);
      }
      // Semaphore handling
      end_tick = xTaskGetTickCount();
      xSemaphoreGive(sensor_sem);

      char ticks[100];
      // snprintf(ticks, sizeof(ticks), "Time taken for bnoRead: %d", (end_tick - start_tick) / portTICK_PERIOD_MS);
      Serial.println(ticks);
      vTaskDelay(bno_delay / portTICK_PERIOD_MS);
    }
  }
}

// SD Card Helper Function
void write_sd_file_headers() {
  Serial.println("Writing SD file headers");
  sd_card.deleteFile(IMU_FILEPATH);
  sd_card.deleteFile(ALTIMETER_FILEPATH);
  sd_card.deleteFile(LORA_FILEPATH);

  sd_card.writeFile(IMU_FILEPATH, "time_stamp, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z\n");

  sd_card.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd_card.writeFile(LORA_FILEPATH, "time_stamp,lora_command\n");
}

// SD card writing task
void sd_card_task(void* parameter) {
  const uint16_t warningThreshold = 100;
  Serial.println("SD card task started");
  SensorData sensor_data;
  while (1) {
    if (xQueueReceive(sdStoreQu, &sensor_data, portMAX_DELAY) == pdTRUE) {
      // Create appropriate filename based on sensor type
      Serial.println("Beginning sd storing process, received from qu");
      String filename;
      String data_str;
      switch (sensor_data.type) {
      case IMU:
        filename = IMU_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," +
          String(sensor_data.data.imu.accel[0]) + "," + String(sensor_data.data.imu.accel[1]) + "," + String(sensor_data.data.imu.accel[2]) + "," +
          String(sensor_data.data.imu.linear[0]) + "," + String(sensor_data.data.imu.linear[1]) + "," + String(sensor_data.data.imu.linear[2]) + "," +
          String(sensor_data.data.imu.gravity[0]) + "," + String(sensor_data.data.imu.gravity[1]) + "," + String(sensor_data.data.imu.gravity[2]) + "," +
          String(sensor_data.data.imu.quat[0]) + "," + String(sensor_data.data.imu.quat[1]) + "," + String(sensor_data.data.imu.quat[2]) + "," + String(sensor_data.data.imu.quat[3]) + "," +
          String(sensor_data.data.imu.gyro[0]) + "," + String(sensor_data.data.imu.gyro[1]) + "," + String(sensor_data.data.imu.gyro[2]);
        break;
      case ALTIMETER:
        filename = ALTIMETER_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.alt.altitude) + "," + String(sensor_data.data.alt.temp) + "," + String(sensor_data.data.alt.pressure);
        break;
      case LORA:
        filename = LORA_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.lora.lora_command);
        break;
      }

      // Write data to SD card
      data_str += "\n";
      sd_card.appendFile(filename.c_str(), data_str.c_str());

      UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
      if (highWaterMark < warningThreshold)
      {
        Serial.print("Warning: sd_card_task is running low on stack memory. High water mark: ");
        Serial.println(highWaterMark);
      }
    }
  }
}

// Logic Tasks --------------------------------
// Protocol Logic Task
void protocol_logic(void* parameters) {
  const uint16_t warningThreshold = 100;
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
        // Received Altimeter On
        Serial.print("Received AO");
        vTaskResume(bmpTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else if (strcmp(command, "AF") == 0) {
        // Received Altimeter Off
        Serial.print("Received AF");
        vTaskSuspend(bmpTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else if (strcmp(command, "IO") == 0) {
        // Received IMU On
        Serial.print("Received IO");
        vTaskResume(bnoTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else if (strcmp(command, "IF") == 0) {
        // Received IMU Off
        Serial.print("Received IF");
        vTaskSuspend(bnoTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else if (strcmp(command, "DA") == 0) {
        // Received Data On
        Serial.print("Received DA");
        vTaskResume(bmpTskHndl);
        vTaskResume(bnoTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else if (strcmp(command, "DF") == 0) {
        // Received Data Off
        Serial.print("Received DF");
        vTaskSuspend(bmpTskHndl);
        vTaskSuspend(bnoTskHndl);
        xQueueReset(Data_HndlQu);
        xQueueReset(loraQu);
      }
      else {
        Serial.print("Received command not recognized");
        txMessage response;
        strncpy(response.strToSend, "Command not recognized", sizeof(response.strToSend) - 1);
        response.addressToSend = 6;
        xQueueSend(loraQu, (void*)&response, portMAX_DELAY);
      }

      UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
      if (highWaterMark < warningThreshold) {
        Serial.print("Warning: protocol_logic task is running low on stack memory. High water mark: ");
        Serial.println(highWaterMark);
      }
    }
  }
}

// Data Handler Task
void data_processing(void* parameters) {
  const uint16_t warningThreshold = 100;
  TickType_t start_tick, end_tick;
  while (1) {
    start_tick = xTaskGetTickCount();
    SensorData sensorData;
    if (xQueueReceive(Data_HndlQu, (void*)&sensorData, portMAX_DELAY) == pdTRUE) {
      // Send the data to the SD Card
      Serial.println("Data Processed to SD");
      xQueueSend(sdStoreQu, &sensorData, portMAX_DELAY);

      // Check if the altimeter data is valid
      if (sensorData.type == ALTIMETER) {
        if (sensorData.data.alt.altitude == 0 && sensorData.data.alt.temp == 0 && sensorData.data.alt.pressure == 0) {
          Serial.println("Invalid altimeter data");
          txMessage alert;
          strncpy(alert.strToSend, "Invalid altimeter data", sizeof(alert.strToSend) - 1);
          alert.addressToSend = 6;
          xQueueSend(loraQu, (void*)&alert, portMAX_DELAY);
        }

        txMessage alt_packet;
        snprintf(alt_packet.strToSend, sizeof(alt_packet.strToSend), "Altitude: %.2fm, %.2f°C, %.2fkPa", sensorData.data.alt.altitude, sensorData.data.alt.temp, sensorData.data.alt.pressure);
        alt_packet.addressToSend = 6;
        xQueueSend(loraQu, (void*)&alt_packet, portMAX_DELAY);
      }

      // Check if the IMU data is valid
      if (sensorData.type == IMU) {
        if (sensorData.data.imu.accel[0] == 0 && sensorData.data.imu.accel[1] == 0 && sensorData.data.imu.accel[2] == 0 && sensorData.data.imu.linear[0] == 0 && sensorData.data.imu.linear[1] == 0 && sensorData.data.imu.linear[2] == 0 && sensorData.data.imu.gravity[0] == 0 && sensorData.data.imu.gravity[1] == 0 && sensorData.data.imu.gravity[2] == 0 && sensorData.data.imu.quat[0] == 0 && sensorData.data.imu.quat[1] == 0 && sensorData.data.imu.quat[2] == 0 && sensorData.data.imu.quat[3] == 0 && sensorData.data.imu.gyro[0] == 0 && sensorData.data.imu.gyro[1] == 0 && sensorData.data.imu.gyro[2] == 0) {
          Serial.println("Invalid IMU data");

          Serial.println("Invalid IMU data");
          txMessage alert;
          strncpy(alert.strToSend, "Invalid IMU data", sizeof(alert.strToSend) - 1);
          alert.addressToSend = 6;
          xQueueSend(loraQu, (void*)&alert, portMAX_DELAY);
        }

        txMessage imu_packet;
        snprintf(imu_packet.strToSend, sizeof(imu_packet.strToSend), "Accel: [X%.2f,Y%.2f,Z%.2f], LinAccel: [X%.2f,Y%.2f,Z%.2f], Gravity: [X%.2f,Y%.2f,Z%.2f], Quat: [W%.2f,X%.2f,Y%.2f,Z%.2f], Gyro: [X%.2f,Y%.2f,Z%.2f]",
          sensorData.data.imu.accel[0], sensorData.data.imu.accel[1], sensorData.data.imu.accel[2],
          sensorData.data.imu.linear[0], sensorData.data.imu.linear[1], sensorData.data.imu.linear[2],
          sensorData.data.imu.gravity[0], sensorData.data.imu.gravity[1], sensorData.data.imu.gravity[2],
          sensorData.data.imu.quat[0], sensorData.data.imu.quat[1], sensorData.data.imu.quat[2], sensorData.data.imu.quat[3],
          sensorData.data.imu.gyro[0], sensorData.data.imu.gyro[1], sensorData.data.imu.gyro[2]);
        imu_packet.addressToSend = 6;
        xQueueSend(loraQu, (void*)&imu_packet, portMAX_DELAY);
      }

      // Check if the altitude has reached a critical threshold
      // Critical thresholds are defined in the config file
      /*if (sensorData.type == ALTIMETER) {
        // First check if the altitude is decreasing
        // Send a crit message once altitude is detected to be decreasing
        if (sensorData.data.alt.altitude < 0) {
          Serial.println("Altitude is decreasing");

          txMessage alt_update;
          strncpy(alt_update.strToSend, "Altitude is decreasing", sizeof(alt_update.strToSend) - 1);
          alt_update.addressToSend = 6;
          xQueueSend(txCritQu, (void *)&alt_update, portMAX_DELAY);
        }

        // Check if the altitude is above 5000m
          if (sensorData.data.alt.altitude > 5000) {
            Serial.println("Altitude is above 5000m");

            txMessage alt_update;
            strncpy(alt_update.strToSend, "Altitude is above 5000m", sizeof(alt_update.strToSend) - 1);
            alt_update.addressToSend = 6;
            xQueueSend(txCritQu, (void *)&alt_update, portMAX_DELAY);
          }

        // Check if the altitude is above 4000m
          if (sensorData.data.alt.altitude > 4000 && sensorData.data.alt.altitude < 5000) {
            Serial.println("Altitude is above 4000m");
            char response[32] = "Altitude is above 4000m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 3000m
          if (sensorData.data.alt.altitude > 3000 && sensorData.data.alt.altitude < 4000) {
            Serial.println("Altitude is above 3000m");
            char response[32] = "Altitude is above 3000m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 2000m
          if (sensorData.data.alt.altitude > 2000 && sensorData.data.alt.altitude < 3000) {
            Serial.println("Altitude is above 2000m");
            char response[32] = "Altitude is above 2000m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 1000m
          if (sensorData.data.alt.altitude > 1000 && sensorData.data.alt.altitude < 2000) {
            Serial.println("Altitude is above 1000m");
            char response[32] = "Altitude is above 1000m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 500m
          if (sensorData.data.alt.altitude > 500 && sensorData.data.alt.altitude < 1000) {
            Serial.println("Altitude is above 500m");
            char response[32] = "Altitude is above 500m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 400m
          if (sensorData.data.alt.altitude > 400 && sensorData.data.alt.altitude < 500) {
            Serial.println("Altitude is above 400m");
            char response[32] = "Altitude is above 400m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 300m
          if (sensorData.data.alt.altitude > 300 && sensorData.data.alt.altitude < 400) {
            Serial.println("Altitude is above 300m");
            char response[32] = "Altitude is above 300m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 200m
          if (sensorData.data.alt.altitude > 200 && sensorData.data.alt.altitude < 300) {
            Serial.println("Altitude is above 200m");
            char response[32] = "Altitude is above 200m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 100m
          if (sensorData.data.alt.altitude > 100 && sensorData.data.alt.altitude < 200) {
            Serial.println("Altitude is above 100m");
            char response[32] = "Altitude is above 100m";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above initial altitude
          if (sensorData.data.alt.altitude > init_altitude) {
            Serial.println("Altitude is above initial altitude");
            char response[64] = "Altitude is above initial altitude";
            xQueueSend(txCritQu, &response, portMAX_DELAY);
          }
      }*/

      end_tick = xTaskGetTickCount();
      char ticks[100];
      // snprintf(ticks, sizeof(ticks), "Time taken for data_processing: %d", (end_tick - start_tick) / portTICK_PERIOD_MS);
      Serial.println(ticks);

      UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
      if (highWaterMark < warningThreshold) {
        Serial.print("Warning: data_processing task is running low on stack memory. High water mark: ");
        Serial.println(highWaterMark);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Setup started");
  // Initialize Peripherals -------------------------------------------------------------
  barometer.Initialize();
  imu_sensor.BNO_SETUP();

  sd_card.begin();
  // Set up SD card files
  write_sd_file_headers();

  // Initialize and send LoRa message
  lora.begin();

  // Create Queues ----------------------------------------------------------------------
  // Transmit Queues
  loraQu = xQueueCreate(LORAQU_LEN, sizeof(txMessage));

  // SD Card Queue
  sdStoreQu = xQueueCreate(SD_CARD_QUEUE_LEN, sizeof(SensorData));

  // Received Commands Queue
  ReceiveQu = xQueueCreate(RECEIVEQU_LEN, sizeof(rxMessage));

  // Data Handling Queue
  Data_HndlQu = xQueueCreate(DATA_HNDL_QU_LEN, sizeof(SensorData));

  // Check if Queues were created successfully
  if (loraQu == NULL || sdStoreQu == NULL || ReceiveQu == NULL || Data_HndlQu == NULL) {
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

  // Create Sensor Semaphore -----------------------------------------------------------
  sensor_sem = xSemaphoreCreateBinary();
  // Check if Semaphore was created successfully
  if (sensor_sem == NULL) {
    Serial.println("Error creating sensor semaphore");
    while (1);
  }
  else {
    Serial.println("Sensor semaphore created successfully");
  }

  // Create BNO Mutex -------------------------------------------------------------------
  bno_mutex = xSemaphoreCreateMutex();
  // Check if Mutex was created successfully
  if (bno_mutex == NULL) {
    Serial.println("Error creating bno mutex");
    while (1);
  }
  else {
    Serial.println("BNO mutex created successfully");
  }

  // Tasks -----------------------------------------------------------------------------
  BaseType_t result;
  // Lora Tasks -----------------------------------------------------
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

  // Sensor Tasks ---------------------------------------------------
  result = xTaskCreatePinnedToCore(
    bmpRead,
    "BMP Read TX",
    8192,
    NULL,
    3,
    &bmpTskHndl,
    app_cpu);
  if (result != pdPASS)
  {
    Serial.println("Error creating bmpRead task");
    while (1)
      ;
  }

  result = xTaskCreatePinnedToCore(
    bnoRead,
    "BNO Read TX",
    8192,
    NULL,
    3,
    &bnoTskHndl,
    app_cpu);
  if (result != pdPASS)
  {
    Serial.println("Error creating bnoRead task");
    while (1)
      ;
  }

  result = xTaskCreatePinnedToCore(
    sd_card_task,
    "SD Card Task",
    8192,
    NULL,
    3,
    &sdTskHndl,
    app_cpu);
  if (result != pdPASS)
  {
    Serial.println("Error creating sd_card_task");
    while (1)
      ;
  }

  // Logic Tasks ----------------------------------------------------
  result = xTaskCreatePinnedToCore(
    protocol_logic,
    "Protocol Logic",
    2048,
    NULL,
    4,
    &prtclTsksHndl,
    app_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating protocol_logic task");
    while (1);
  }

  result = xTaskCreatePinnedToCore(
    data_processing,
    "Data Processing",
    8192,
    NULL,
    4,
    &DtaTskHndl,
    app_cpu);
  if (result != pdPASS)
  {
    Serial.println("Error creating data_processing task");
    while (1)
      ;
  }

  Serial.println("setup completed and giving semaphore");

  // Give the semaphores initially to start the tasks
  xSemaphoreGive(lora_sem);
  xSemaphoreGive(sensor_sem);

  txMessage wakeup;
  strncpy(wakeup.strToSend, "WU", sizeof(wakeup.strToSend) - 1);
  wakeup.addressToSend = 6;
  xQueueSend(loraQu, (void*)&wakeup, portMAX_DELAY);
}

void loop()
{

}
