#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
static const BaseType_t WiFi_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
static const BaseType_t WiFi_cpu = 0;
#endif

#ifndef LED_BUILTIN
// #define LED_BUILTIN 2
#endif

#include "V2_1_SOAR_RTOS_IMU.h"
#include "V1_SOAR_RTC.h"
#include "V1_SOAR_RTOS_BAROMETER.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "sensor_data_types.h"
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "XBeeArduino.h"
#include "WiFi.h"
#include "esp_wifi.h"
#include <NetworkClient.h>
#include <WiFiAP.h>


// Pavan doesn't know what this is
int byteMax = 8192 * 4;

// Global Variables -------------------------------------------------------------------------------
// Instances of classes
SOAR_RTC timer;
SOAR_IMU imu_sensor;
SOAR_BAROMETER barometer;
SOAR_RTOS_GPS gps;
SOAR_SD_CARD sd(D0);

typedef struct {
  char text[256]; // Buffer to hold the string
} XBeeMsg;

// RTOS Handles -----------------------------------------------------
static QueueHandle_t SensQu;
static QueueHandle_t XBee_Qu;

// WiFi Credentials -------------------------------------------
const char *ssid = "SoarMonkey";
const char *password = "IAmAMonkey";
NetworkServer server(80);
String receivedData = "";
const char* RECEIVE_ENDPOINT = "/send-data";

// SD CSV File Paths ------------------------------------------------
const char* IMU_FILEPATH = "/imu.csv";
const char* ALTIMETER_FILEPATH = "/altimeter.csv";
const char* GPS_FILEPATH = "/gps.csv";
const char* PI_FILEPATH = "/pi.csv";

// XBee setup -------------------------------------------------------
HardwareSerial mySerial(0);
Stream* serialPort = &mySerial;
XBeeArduino* xbee;

// XBee Stuff -------------------------------------------------------------------------------------
void XBeeTask(void* Parameters) {
  while (1) {
    XBeeMsg transmission;
    if (xQueueReceive(XBee_Qu, &transmission, portMAX_DELAY) == pdTRUE) {
      Serial.print("XBeeTask: Data to send: ");
      Serial.println(transmission.text);

      if (xbee->isConnected()) {
        if (!xbee->completeTx(transmission.text)) {
          Serial.println("Failed to send data");
        }
        else {
          Serial.println("XBeeTask: Data sent successfully");
        }
      }
    }
  }
}

void OnReceiveCallback(void* data)
{
  Serial.println("Onreceive callback executed");

}

void OnSendCallback(void* data)
{
  Serial.println("Send callback initiated");
}

// IMU Task ---------------------------------------------------------------------------------------
void GetIMUTask(void* Parameters) {
  while (1) {
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

// Altimeter Task ---------------------------------------------------------------------------------
void GetALTTask(void* Parameters) {
  while (1) {
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

// GPS Task ---------------------------------------------------------------------------------------
void GetGPSTask(void* Parameters) {
  while (1) {
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

    static char stat[20];
    static char lat[20];
    static char N_S[20];
    static char longitude[20];
    static char E_W[20];
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

    Serial.println(gps_data.data.gps.status);
    Serial.println(gps_data.data.gps.lat);
    Serial.println(gps_data.data.gps.N_S);
    Serial.println(gps_data.data.gps.longitude);
    Serial.println(gps_data.data.gps.E_W);
    xQueueSend(SensQu, &gps_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// SD Card Task -----------------------------------------------------------------------------------
void ReadSensorTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = byteMax * 0.98;
    SensorData all_sensors;
    // Waits until data is ready to send to SD Card
    if (xQueueReceive(SensQu, &all_sensors, portMAX_DELAY) == pdTRUE) {
      // Prepares the storage variables
      String fileIMU;
      String fileALTIMETER;
      String fileGPS;
      String filePI;
      String data_str_IMU;
      String data_str_ALTIMETER;
      String data_str_GPS;
      String data_str_PI;
      String final_transmission;

      // Builds the data strings ------------------------------------
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


        final_transmission =
          // SenderID,RxID
          String("2,4,") +
          // Hours,Minutes,Seconds,Microseconds
          String(all_sensors.data.imu.hours) + "," + String(all_sensors.data.imu.minutes) + "," + String(all_sensors.data.imu.seconds) + "," + String(all_sensors.data.imu.microseconds) + "," +
          // AccelX,AccelY,AccelZ
          String(all_sensors.data.imu.accel[0]) + "," + String(all_sensors.data.imu.accel[1]) + "," + String(all_sensors.data.imu.accel[2]) + "," +
          // LinearX,LinearY,LinearZ
          String(all_sensors.data.imu.linear[0]) + "," + String(all_sensors.data.imu.linear[1]) + "," + String(all_sensors.data.imu.linear[2]) + "," +
          // GravityX,GravityY,GravityZ
          String(all_sensors.data.imu.gravity[0]) + "," + String(all_sensors.data.imu.gravity[1]) + "," + String(all_sensors.data.imu.gravity[2]) + "," +
          // QuatW,QuatX,QuatY,QuatZ
          String(all_sensors.data.imu.quat[0]) + "," + String(all_sensors.data.imu.quat[1]) + "," + String(all_sensors.data.imu.quat[2]) + "," + String(all_sensors.data.imu.quat[3]) + "," +
          // GyroX,GyroY,GyroZ
          String(all_sensors.data.imu.gyro[0]) + "," + String(all_sensors.data.imu.gyro[1]) + "," + String(all_sensors.data.imu.gyro[2]) +
          // Altitude,Temperature,Pressure
          String("-,-,-") + "," +
          // Status,Latitude,N_S,Longitude,E_W
          String("-,-,-,-,-") + "," +
          // Pi Message
          String(",");
        break;

      case ALTIMETER:
        fileALTIMETER = ALTIMETER_FILEPATH;
        data_str_ALTIMETER =
          String(all_sensors.data.alt.hours) + "," + String(all_sensors.data.alt.minutes) + "," + String(all_sensors.data.alt.seconds) + "," + String(all_sensors.data.alt.microseconds) + "," +
          String(all_sensors.data.alt.altitude) + "," + String(all_sensors.data.alt.temp) + "," + String(all_sensors.data.alt.pressure);

        final_transmission =
          // SenderID,RxID
          String("2,4,") +
          // Hours,Minutes,Seconds,Microseconds
          String(all_sensors.data.alt.hours) + "," + String(all_sensors.data.alt.minutes) + "," + String(all_sensors.data.alt.seconds) + "," + String(all_sensors.data.alt.microseconds) + "," +
          // AccelX,AccelY,AccelZ
          String("-,-,-") + "," +
          // LinearX,LinearY,LinearZ
          String("-,-,-") + "," +
          // GravityX,GravityY,GravityZ
          String("-,-,-") + "," +
          // QuatW,QuatX,QuatY,QuatZ
          String("-,-,-,-") + "," +
          // GyroX,GyroY,GyroZ
          String("-,-,-") + "," +
          // Altitude,Temperature,Pressure
          String(all_sensors.data.alt.altitude) + "," + String(all_sensors.data.alt.temp) + "," + String(all_sensors.data.alt.pressure) + "," +
          // Status,Latitude,N_S,Longitude,E_W
          String("-,-,-,-,-") + "," +
          // Pi Message
          String(",");
        break;

      case GPS:
        fileGPS = GPS_FILEPATH;
        data_str_GPS =
          String(all_sensors.data.gps.hours) + "," + String(all_sensors.data.gps.minutes) + "," + String(all_sensors.data.gps.seconds) + "," + String(all_sensors.data.gps.microseconds) + "," +
          String(all_sensors.data.gps.status) + "," + String(all_sensors.data.gps.lat) + "," + String(all_sensors.data.gps.N_S) + "," + String(all_sensors.data.gps.longitude) + "," + String(all_sensors.data.gps.E_W);

        final_transmission =
          // SenderID,RxID
          String("2,4,") +
          // Hours,Minutes,Seconds,Microseconds
          String(all_sensors.data.gps.hours) + "," + String(all_sensors.data.gps.minutes) + "," + String(all_sensors.data.gps.seconds) + "," + String(all_sensors.data.gps.microseconds) + "," +
          // AccelX,AccelY,AccelZ
          String("-,-,-") + "," +
          // LinearX,LinearY,LinearZ
          String("-,-,-") + "," +
          // GravityX,GravityY,GravityZ
          String("-,-,-") + "," +
          // QuatW,QuatX,QuatY,QuatZ
          String("-,-,-,-") + "," +
          // GyroX,GyroY,GyroZ
          String("-,-,-") + "," +
          // Altitude,Temperature,Pressure
          String("-,-,-") + "," +
          // Status,Latitude,N_S,Longitude,E_W
          String(all_sensors.data.gps.status) + "," + String(all_sensors.data.gps.lat) + "," + String(all_sensors.data.gps.N_S) + "," + String(all_sensors.data.gps.longitude) + "," + String(all_sensors.data.gps.E_W) + "," +
          // Pi Message
          String(",");
        break;
      case PI_R:
        filePI = PI_FILEPATH;
        data_str_PI =
          // String(all_sensors.data.pi_r.hours) + "," + String(all_sensors.data.pi_r.minutes) + "," + String(all_sensors.data.pi_r.seconds) + "," + String(all_sensors.data.pi_r.microseconds) + "," +
          String(all_sensors.data.pi_r.pidata);

        final_transmission =
          // SenderID,RxID
          String("2,4,") +
          // Hours,Minutes,Seconds,Microseconds
          // String(all_sensors.data.pi_r.hours) + "," + String(all_sensors.data.pi_r.minutes) + "," + String(all_sensors.data.pi_r.seconds) + "," + String(all_sensors.data.pi_r.microseconds) + "," +
          String("-,-,-,-") + "," +
          // AccelX,AccelY,AccelZ
          String("-,-,-") + "," +
          // LinearX,LinearY,LinearZ
          String("-,-,-") + "," +
          // GravityX,GravityY,GravityZ
          String("-,-,-") + "," +
          // QuatW,QuatX,QuatY,QuatZ
          String("-,-,-,-") + "," +
          // GyroX,GyroY,GyroZ
          String("-,-,-") + "," +
          // Altitude,Temperature,Pressure
          String("-,-,-") + "," +
          // Status,Latitude,N_S,Longitude,E_W
          String("-,-,-,-,-") + "," +
          // Pi Message
          String(all_sensors.data.pi_r.pidata);
        break;
      }

      data_str_IMU += "\n";
      data_str_ALTIMETER += "\n";
      data_str_GPS += "\n";
      data_str_PI += "\n";
      final_transmission += "\n";

      // Append the data to the SD Card -----------------------------
      // Serial.println("Writes to SD Card: IMU");
      sd.appendFile(fileIMU.c_str(), data_str_IMU.c_str());
      // Serial.println("Writes to SD Card: Altimeter");
      sd.appendFile(fileALTIMETER.c_str(), data_str_ALTIMETER.c_str());
      // Serial.println("Writes to SD Card: GPS");
      sd.appendFile(fileGPS.c_str(), data_str_GPS.c_str());
      sd.appendFile(filePI.c_str(), data_str_PI.c_str());

      // Send the data to the XBee ----------------------------------
      Serial.print("Final Transmission: ");
      Serial.println(final_transmission.c_str());

      XBeeMsg message;
      final_transmission.toCharArray(message.text, sizeof(message.text)); // Convert String to char array
      xQueueSend(XBee_Qu, &message, portMAX_DELAY);


      // Testing Purposes, print altitude
      // Serial.println(all_sensors.data.alt.altitude);


    }
  }
}

void WiFiTask(void* Parameters) {
  while(1) {
      NetworkClient client = server.accept();
      
      if (client) {
          Serial.println("New Client.");
          String currentLine = "";
          while (client.connected()) {
              if (client.available()) {
                  char c = client.read();
                  Serial.write(c);
                  
                  if (c == '\n') {
                      if (currentLine.length() == 0) {
                          // Send HTTP response
                          client.println("HTTP/1.1 200 OK");
                          client.println("Content-type:text/html");
                          client.println();

                          // Send webpage with form
                          client.println("<html><body>");
                          client.println("<h1>SOAR Telemetry Bay</h1>");
                          
                          // Add form for sending data
                          client.println("<form action='/send-data' method='POST'>");
                          client.println("Data: <input type='text' name='data'><br>");
                          client.println("<input type='submit' value='Send Data'>");
                          client.println("</form>");
                          
                          // Show received data
                          client.println("<h2>Last Received Data:</h2>");
                          client.println(receivedData);
                          
                          client.println("</body></html>");
                          client.println();
                          break;
                      } else {
                          // Check if this is a POST request with data
                          if (currentLine.startsWith("POST /send-data")) {
                              // Wait for the POST data
                              while (client.available()) {
                                  String line = client.readStringUntil('\n');
                                  if (line.indexOf("data=") != -1) {
                                      // Extract and store the data
                                      receivedData = line.substring(line.indexOf("=") + 1);
                                    
                                      SensorData pi_data;
                                      pi_data.type = PI_R;

                                      // int current_hours = timer.getTimeHours();
                                      // int current_minutes = timer.getTimeMinutes();
                                      // int current_seconds = timer.getTimeSeconds();
                                      // int current_microseconds = timer.getTimeMicroseconds();
                                      // pi_data.data.pi_r.hours = current_hours;
                                      // pi_data.data.pi_r.minutes = current_minutes;
                                      // pi_data.data.pi_r.seconds = current_seconds;
                                      // pi_data.data.pi_r.microseconds = current_microseconds;

                                      // Update the struct
                                      receivedData.toCharArray(pi_data.data.pi_r.pidata, sizeof(pi_data.data.pi_r.pidata));

                                      xQueueSend(SensQu, &pi_data, portMAX_DELAY);
                                      vTaskDelay(pdMS_TO_TICKS(500));
                                  }
                              }
                          }
                          currentLine = "";
                      }
                  } else if (c != '\r') {
                      currentLine += c;
                  }
              }
          }
          client.stop();
          Serial.println("Client Disconnected.");
      }
      vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to prevent watchdog triggers
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize the sensors -----------------------------------------------------------------------
  barometer.Initialize();

  // vTaskDelay(300); 

  imu_sensor.BNO_SETUP();

  // vTaskDelay(300); 

  gps.setup();

  // vTaskDelay(300); 

  timer.adjustTime(4, 3, 2025, 18, 41, 00, 000000);

  // vTaskDelay(300);

  // WiFi Initialize ------------------------------------------------------------------------------
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();

  // Initialize the SD Card -----------------------------------------
  sd.begin();
  sd.writeFile(IMU_FILEPATH, "hours, minutes, seconds, microseconds, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "hours, minutes, seconds, microseconds, altitude, temperature, pressure\n");
  sd.writeFile(GPS_FILEPATH, "hours, minutes, seconds, microseconds, status, latitude, North/South, longitude, East/West\n");
  sd.writeFile(PI_FILEPATH, "Pi Message\n");

  // Initial Strings ------------------------------------------------
  String initialIMU = "Initializing IMU";
  String initialALTIMETER = "Intializing Altimeter";
  String initialGPS = "Initializing GPS";
  String initialPI = "Initializing PI";
  String ifileIMU = IMU_FILEPATH;
  String ifileALTIMETER = ALTIMETER_FILEPATH;
  String ifileGPS = GPS_FILEPATH;
  String ifilePI = PI_FILEPATH;

  // Starting new iteration -----------------------------------------
  initialIMU += "\n";
  initialALTIMETER += "\n";
  initialGPS += "\n";
  initialPI += "\n";
  sd.appendFile(ifileIMU.c_str(), initialIMU.c_str());
  sd.appendFile(ifileALTIMETER.c_str(), initialALTIMETER.c_str());
  sd.appendFile(ifileGPS.c_str(), initialGPS.c_str());
  sd.appendFile(ifilePI.c_str(), initialPI.c_str());


  // Initialize the XBee --------------------------------------------------------------------------
  xbee = new XBeeArduino(serialPort, 115200, XBEE_STANDARD, OnReceiveCallback, OnSendCallback);
  mySerial.begin(115200, SERIAL_8N1, -1, -1);

  if (!xbee->begin()) {
    Serial.println("Failed to initialize XBee");
    while (1)
      ; // Halt the program on failure
  }
  WiFi.setSleep(false);
  Wire.setClock(100000);  // Lower I2C clock speed for stability


  // RTOS Initialization --------------------------------------------------------------------------
  // Initialize Queues ----------------------------------------------
  SensQu = xQueueCreate(10, sizeof(SensorData));
  // Make XBee_Qu hold String, not SensorData
  XBee_Qu = xQueueCreate(10, sizeof(String));
  XBee_Qu = xQueueCreate(10, sizeof(XBeeMsg)); // Changed to XBeeMsg type


  // Create Tasks ---------------------------------------------------
  xTaskCreatePinnedToCore(
    GetIMUTask,
    "Get IMU Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );
  xTaskCreatePinnedToCore(
    GetALTTask,
    "Get Altimeter Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    GetGPSTask,
    "Get GPS Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    XBeeTask,
    "XBee Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );

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

  // Create the Read Sensor Task
  xTaskCreatePinnedToCore(
    WiFiTask,
    "WiFi Task",
    byteMax,
    NULL,
    3,
    NULL,
    WiFi_cpu
  );
}

void loop() {

}