// #include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "V2_1_SOAR_RTOS_IMU.h"
#include "V1_SOAR_RTC.h"
#include "V1_SOAR_RTOS_BAROMETER.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "sensor_data_types.h"
#include "V1_SOAR_RTOS_SD_CARD.h"

int byteMax = 8192 * 2;
SOAR_RTC timer;
SOAR_IMU imu_sensor;
SOAR_BAROMETER barometer;
SOAR_RTOS_GPS gps;
SOAR_SD_CARD sd(D0);
static QueueHandle_t SensQu;
const char* IMU_FILEPATH = "/imu.csv";
const char* ALTIMETER_FILEPATH = "/altimeter.csv";
const char* GPS_FILEPATH = "/gps.csv";

void GetIMUTask(void* Parameters) {
  while(1) {    
    // IMU Data -------------------------------------------------------------------------------
        SensorData imu_data;
        imu_data.type = IMU;
        int current_hours = timer.getTimeHours();
        int current_minutes = timer.getTimeMinutes();
        int current_seconds = timer.getTimeSeconds();
        int current_microseconds = timer.getTimeMicroseconds();
        imu_data.data.imu.hours = current_hours;
        imu_data.data.imu.minutes = current_minutes;
        imu_data.data.imu.seconds = current_seconds;
        imu_data.data.imu.microseconds = current_microseconds;
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
        imu_data.data.imu.accel[0] = accel[0];
        imu_data.data.imu.accel[1] = accel[1]; // accel y
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

        xQueueSend(SensQu, &imu_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
} 
void GetALTTask(void* Parameters) {
  while(1) {
  // Altimeter Data -------------------------------------------------------------------------
      SensorData altimeter_data;
      altimeter_data.type = ALTIMETER;

      int current_hours = timer.getTimeHours();
      int current_minutes = timer.getTimeMinutes();
      int current_seconds = timer.getTimeSeconds();
      int current_microseconds = timer.getTimeMicroseconds();
      altimeter_data.data.alt.hours = current_hours;
      altimeter_data.data.alt.minutes = current_minutes;
      altimeter_data.data.alt.seconds = current_seconds;
      altimeter_data.data.alt.microseconds = current_microseconds;

      // Initialize Altimeter Variables
      float altimeter = barometer.get_altitude();
      float temp = barometer.get_temperature();
      float pressure = barometer.get_pressure();

      // Update the struct
      altimeter_data.data.alt.altitude = altimeter;
      altimeter_data.data.alt.temp = temp;
      altimeter_data.data.alt.pressure = pressure;

      xQueueSend(SensQu, &altimeter_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void GetGPSTask(void* Parameters) {
  while(1) {
    // GPS Data -------------------------------------------------------------------------------
    // Get the GPS NMEA Sentence
        SensorData gps_data;
        gps_data.type = GPS;

        int current_hours = timer.getTimeHours();
        int current_minutes = timer.getTimeMinutes();
        int current_seconds = timer.getTimeSeconds();
        int current_microseconds = timer.getTimeMicroseconds();
        gps_data.data.gps.hours = current_hours;
        gps_data.data.gps.minutes = current_minutes;
        gps_data.data.gps.seconds = current_seconds;
        gps_data.data.gps.microseconds = current_microseconds;

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
        gps_data.data.gps.status = i_status;
        gps_data.data.gps.lat = i_lat;
        gps_data.data.gps.N_S = i_N_S;
        gps_data.data.gps.longitude = i_longitude;
        gps_data.data.gps.E_W = i_E_W;

        xQueueSend(SensQu, &gps_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
} 

void ReadSensorTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = byteMax * 0.98;
    SensorData all_sensors;
    // Store in the SD Card -------------------------------------------------------------------
    if (xQueueReceive(SensQu, &all_sensors, portMAX_DELAY) == pdTRUE) {
      String fileIMU;
      String fileALTIMETER;
      String fileGPS;
      String data_str_IMU;
      String data_str_ALTIMETER;
      String data_str_GPS;
      switch (all_sensors.type) {
        case IMU:
        fileIMU = IMU_FILEPATH;
        data_str_IMU = 
        String(all_sensors.data.imu.hours) + "," + String(all_sensors.data.imu.minutes) + "," + String(all_sensors.data.imu.seconds) + "," + String(all_sensors.data.imu.microseconds) + "," +
        String(all_sensors.data.imu.accel[0]) + "," + String(all_sensors.data.imu.accel[1]) + "," + String(all_sensors.data.imu.accel[2]) + "," +
        String(all_sensors.data.imu.linear[0]) + "," + String(all_sensors.data.imu.linear[1]) + "," + String(all_sensors.data.imu.linear[2]) + "," +
        String(all_sensors.data.imu.gravity[0]) + "," + String(all_sensors.data.imu.gravity[1]) + "," + String(all_sensors.data.imu.gravity[2]) + "," +
        String(all_sensors.data.imu.quat[0]) + "," + String(all_sensors.data.imu.quat[1]) + "," + String(all_sensors.data.imu.quat[2]) + "," + String(all_sensors.data.imu.quat[3]) + "," +
        String(all_sensors.data.imu.gyro[0]) + "," + String(all_sensors.data.imu.gyro[1]) + "," + String(all_sensors.data.imu.gyro[2]);
        break;
        
        case ALTIMETER: 
        fileALTIMETER = ALTIMETER_FILEPATH;
        data_str_ALTIMETER = 
        String(all_sensors.data.alt.hours) + "," + String(all_sensors.data.alt.minutes) + "," + String(all_sensors.data.alt.seconds) + "," + String(all_sensors.data.alt.microseconds) + "," +
        String(all_sensors.data.alt.altitude) + "," + String(all_sensors.data.alt.temp) + "," + String(all_sensors.data.alt.pressure);
        break;

        case GPS:
        fileGPS = GPS_FILEPATH;
        data_str_GPS = 
        String(all_sensors.data.gps.hours) + "," + String(all_sensors.data.gps.minutes) + "," + String(all_sensors.data.gps.seconds) + "," + String(all_sensors.data.gps.microseconds) + "," +
        String(all_sensors.data.gps.status) + "," + String(all_sensors.data.gps.lat) + "," + String(all_sensors.data.gps.N_S) + "," + String(all_sensors.data.gps.longitude) + "," + String(all_sensors.data.gps.E_W);
        break;
      }

      data_str_IMU += "\n";
      data_str_ALTIMETER += "\n";
      data_str_GPS += "\n";
      sd.appendFile(fileIMU.c_str(), data_str_IMU.c_str());
      sd.appendFile(fileALTIMETER.c_str(), data_str_ALTIMETER.c_str());
      sd.appendFile(fileGPS.c_str(), data_str_GPS.c_str());
      Serial.println(all_sensors.data.alt.altitude);
      }
  }
  }

void setup() {
  Serial.begin(115200);
  barometer.Initialize();
  imu_sensor.BNO_SETUP();
  gps.setup();
  timer.adjustTime(4, 3, 2025, 18, 41, 00, 000000);

  // Initialize the SD Card
  sd.begin();
  sd.writeFile(IMU_FILEPATH, "hours, minutes, seconds, microseconds, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "hours, minutes, seconds, microseconds, altitude, temperature, pressure\n");
  sd.writeFile(GPS_FILEPATH, "hours, minutes, seconds, microseconds, status, latitude, North/South, longitude, East/West\n");

  // New itation string
  String initialIMU = "Initializing IMU";
  String initialALTIMETER = "Intializing Altimeter";
  String initialGPS = "Initializing GPS";
  String ifileIMU = IMU_FILEPATH;
  String ifileALTIMETER = ALTIMETER_FILEPATH;
  String ifileGPS = GPS_FILEPATH;


  // Starting new iteration
  initialIMU += "\n";
  initialALTIMETER += "\n";
  initialGPS += "\n";
  sd.appendFile(ifileIMU.c_str(), initialIMU.c_str());
  sd.appendFile(ifileALTIMETER.c_str(), initialALTIMETER.c_str());
  sd.appendFile(ifileGPS.c_str(), initialGPS.c_str());
  

  // Create the sensor data queue
  SensQu = xQueueCreate(10, sizeof(SensorData));

  xTaskCreate(
    GetIMUTask,
    "Get IMU Task",
    byteMax,
    NULL,
    2,
    NULL
  );
  xTaskCreate(
    GetALTTask,
    "Get Altimeter Task",
    byteMax,
    NULL,
    2,
    NULL
  );

  xTaskCreate(
    GetGPSTask,
    "Get GPS Task",
    byteMax,
    NULL,
    2,
    NULL
  );

  // Create the Read Sensor Task
  xTaskCreatePinnedToCore(
    ReadSensorTask,
    "Read Sensor Task",
    byteMax,
    NULL,
    3,
    NULL
  );
}

void loop() {

}