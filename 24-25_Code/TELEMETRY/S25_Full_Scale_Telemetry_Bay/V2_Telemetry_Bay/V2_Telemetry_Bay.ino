#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// #include <Wire.h>
#include "V2_1_SOAR_RTOS_IMU.h"
#include "V1_SOAR_RTC.h"
// #include "V1_SOAR_RTOS_BAROMETER.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "sensor_data_types.h"
#include "V1_SOAR_RTOS_SD_CARD.h"

int byteMax = 8192;
SOAR_RTC timer;
SOAR_IMU imu_sensor;
// SOAR_BAROMETER barometer;
SOAR_RTOS_GPS gps;
SOAR_SD_CARD sd(D0);
static QueueHandle_t SensQu;
SemaphoreHandle_t sd_mutex;
const char* ALL_SENSORS_FILEPATH = "/all_sensors.csv";



void ReadSensorTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = byteMax * 0.98;
    SensorData all_sensors;
    // Create timestamp --------------------------------------------------------------
    char date[20];
    char time[20];
    char* current_date = timer.getDate(date);
    char* current_time = timer.getTimeInt(time);

    // Update the Struct
    all_sensors.data.rtc.date = current_date;
    all_sensors.data.rtc.time = current_time;
    // all_sensors.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

    // IMU Data -------------------------------------------------------------------------------
    imu_sensor.loop_iterations++;

    // Initialize IMU Variables
    float accel[3];
    float lin_accel[3];
    float gravity[3];
    float quat[4];
    float gyro[3];

    // Call IMU Methods
    imu_sensor.GET_ACCELERATION(accel);
    imu_sensor.GET_LINEARACCEL(lin_accel);
    imu_sensor.GET_GRAVITY(gravity);
    imu_sensor.GET_QUAT(quat);
    imu_sensor.GET_GYROSCOPE(gyro);

    // Update the struct
    all_sensors.data.imu.accel[0] = accel[0];
    all_sensors.data.imu.accel[1] = accel[1]; // accel y
    all_sensors.data.imu.accel[2] = accel[2];
    all_sensors.data.imu.linear[0] = lin_accel[0];
    all_sensors.data.imu.linear[1] = lin_accel[1];
    all_sensors.data.imu.linear[2] = lin_accel[2];
    all_sensors.data.imu.gravity[0] = gravity[0];
    all_sensors.data.imu.gravity[1] = gravity[1];
    all_sensors.data.imu.gravity[2] = gravity[2];
    all_sensors.data.imu.quat[0] = quat[0];
    all_sensors.data.imu.quat[1] = quat[1];
    all_sensors.data.imu.quat[2] = quat[2];
    all_sensors.data.imu.quat[3] = quat[3];
    all_sensors.data.imu.gyro[0] = gyro[0];
    all_sensors.data.imu.gyro[1] = gyro[1];
    all_sensors.data.imu.gyro[2] = gyro[2];
    // all_sensors.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

    // Altimeter Data -------------------------------------------------------------------------

    // Initialize Altimeter Variables
    // float altimeter = barometer.get_altitude();
    // float temp = barometer.get_temperature();
    // float pressure = barometer.get_pressure();

    // // Update the struct
    // all_sensors.data.alt.altitude = altimeter;
    // all_sensors.data.alt.temp = temp;
    // all_sensors.data.alt.pressure = pressure;
    // all_sensors.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

    // GPS Data -------------------------------------------------------------------------------
    // Get the GPS NMEA Sentence
    char stat[6];
    char lat[6];
    char N_S[1];
    char longitude[6];
    char E_W[1];
    char* i_status = gps.GET_NMEA(stat, 2);
    char* i_lat = gps.GET_NMEA(lat, 3);
    char* i_N_S = gps.GET_NMEA(N_S, 4);
    char* i_longitude = gps.GET_NMEA(longitude, 5);
    char* i_E_W = gps.GET_NMEA(E_W, 6);


    // Update the struct
    all_sensors.data.gps.status = i_status;
    all_sensors.data.gps.lat = i_lat;
    all_sensors.data.gps.N_S = i_N_S;
    all_sensors.data.gps.longitude = i_longitude;
    all_sensors.data.gps.E_W = i_E_W;


    // strncpy(all_sensors.data.gps.nmea, nmea, sizeof(all_sensors.data.gps.nmea) - 1);
    // all_sensors.data.gps.nmea[sizeof(all_sensors.data.gps.nmea) - 1] = '\0'; // Ensure null-termination

    // Store in the SD Card -------------------------------------------------------------------
    if (xSemaphoreTake(sd_mutex, portMAX_DELAY) == pdTRUE) {
      String filename = ALL_SENSORS_FILEPATH;
      String data_str;

      data_str = String(all_sensors.data.rtc.date) + "," + String(all_sensors.data.rtc.time) + "," +
        String(all_sensors.data.imu.accel[0]) + "," + String(all_sensors.data.imu.accel[1]) + "," + String(all_sensors.data.imu.accel[2]) + "," +
        String(all_sensors.data.imu.linear[0]) + "," + String(all_sensors.data.imu.linear[1]) + "," + String(all_sensors.data.imu.linear[2]) + "," +
        String(all_sensors.data.imu.gravity[0]) + "," + String(all_sensors.data.imu.gravity[1]) + "," + String(all_sensors.data.imu.gravity[2]) + "," +
        String(all_sensors.data.imu.quat[0]) + "," + String(all_sensors.data.imu.quat[1]) + "," + String(all_sensors.data.imu.quat[2]) + "," + String(all_sensors.data.imu.quat[3]) + "," +
        String(all_sensors.data.imu.gyro[0]) + "," + String(all_sensors.data.imu.gyro[1]) + "," + String(all_sensors.data.imu.gyro[2]) + "," +
        // String(all_sensors.data.alt.altitude) + "," + 
        // String(all_sensors.data.alt.temp) + "," + 
        // String(all_sensors.data.alt.pressure) + "," + 
        String(all_sensors.data.gps.status) + "," + String(all_sensors.data.gps.lat) + "," + String(all_sensors.data.gps.N_S) + "," + String(all_sensors.data.gps.longitude) + "," + String(all_sensors.data.gps.E_W);

      data_str += "\n";
      sd.appendFile(filename.c_str(), data_str.c_str());
      xSemaphoreGive(sd_mutex);
    }

    // Send to the sensor queue --------------------------------------------------------------------------------------------
    xQueueSend(SensQu, &all_sensors, portMAX_DELAY);
    // vTaskDelay(200 / portTICK_PERIOD_MS);
    // UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    // if (highWaterMark > warningThreshold)
    // {
    //   Serial.print("Warning: sensorRead task is running low on stack memory. High water mark: ");
    //   Serial.println(highWaterMark);
    // }
  }
}

void SensQuReadTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = 8000;
    SensorData sensor_data;
    if (xQueueReceive(SensQu, &sensor_data, portMAX_DELAY) == pdTRUE) {
      // Serial.println(sensor_data.data.alt.altitude);
      // Serial.println(sensor_data.data.imu.accel[1]);
      Serial.println(sensor_data.data.rtc.date);
      Serial.println(sensor_data.data.rtc.time);
      Serial.println(sensor_data.data.gps.status);
      Serial.println(sensor_data.data.gps.lat);
      Serial.println(sensor_data.data.gps.N_S);
      Serial.println(sensor_data.data.gps.longitude);
      Serial.println(sensor_data.data.gps.E_W);
      Serial.println(sizeof(sensor_data));
      // UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
      // if (highWaterMark > warningThreshold)
      // {
      //   Serial.print("Warning: sd task is running low on stack memory. High water mark: ");
      //   Serial.println(highWaterMark);
      // }
    }


    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  // barometer.Initialize();
  imu_sensor.BNO_SETUP();
  gps.setup();
  timer.adjustTime(3, 6, 2025, 22, 35, 30, 000000);

  // Initialize the SD Card
  sd.begin();
  sd.deleteFile(ALL_SENSORS_FILEPATH);
  sd.writeFile(ALL_SENSORS_FILEPATH, "date, time, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z, status, latitude, North/South, longitude, East/West\n");
  // Create the sensor data queue
  SensQu = xQueueCreate(10, sizeof(SensorData));
  sd_mutex = xSemaphoreCreateMutex();

  // Create the Read Sensor Task
  xTaskCreatePinnedToCore(
    ReadSensorTask,
    "Read Sensor Task",
    byteMax,
    NULL,
    3,
    NULL,
    app_cpu
  );

  // Create the Sensor Queue Read Task
  xTaskCreatePinnedToCore(
    SensQuReadTask,
    "Sensor Queue Read Task",
    2096,
    NULL,
    3,
    NULL,
    app_cpu
  );
}

void loop() {
  // Empty, tasks are handled by FreeRTOS
}