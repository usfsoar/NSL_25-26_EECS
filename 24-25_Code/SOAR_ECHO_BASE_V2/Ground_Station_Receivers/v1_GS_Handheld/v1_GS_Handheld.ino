#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include <string.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "V1_SOAR_RTC.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "v1_SOAR_RTOS_DISPLAY.h"
#include "XBeeArduino.h"
#include "sensor_data_types.h"
#include <cmath>

// Add before setup()
#define TFT_CS   5
#define TFT_DC   21
#define TFT_MOSI 23
#define TFT_SCK  18
#define TFT_RST  22
#define TFT_MISO 19

// Globals ----------------------------------------------------------------------------------------
// Instances of classes
SOAR_RTC timer;
SOAR_RTOS_GPS gps;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);
SOAR_RTOS_DISPLAY disp(tft);
SOAR_SD_CARD sd(D0);

// RTOS Handles -----------------------------------------------------
TaskHandle_t XBeeHandle;
// TaskHandle_t

// Queues -----------------------------------------------------------
static QueueHandle_t XBee_Qu;
typedef struct {
  char text[256]; // Buffer to hold the string
} XBeeMsg;

// SD CSV File Paths ------------------------------------------------
const char* GPS_FILEPATH = "/gps.csv";
const char* TELEMETRY_FILEPATH = "/telemetry.csv";

// XBee setup -------------------------------------------------------
HardwareSerial mySerial(0);
Stream* serialPort = &mySerial;
XBeeArduino* xbee;

// Tasks ------------------------------------------------------------------------------------------
// XBee Stuff -------------------------------------------------------
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
    vTaskDelay(50); // Add a small delay to prevent busy-waiting
    xbee->process(); // Process incoming data
    // WARNING: NEED TO ADD A THING TO USE THE ONRECEIVE CALLBACK
  }
}

void OnReceiveCallback(void* data) {
  Serial.println("Onreceive callback executed");
}

void OnSendCallback(void* data) {
  Serial.println("Send callback initiated");
}


// MegaTask  ----------------------------------------------------
void MegaTask(void* Parameters) {
  while (1) {
    // takes inputs from xbee and feeds to sd card and display

    // Get the GPS Data ---------------------------------------------------------------------------
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

    char stat[20];
    char lat[20];
    char N_S[20];
    char longitude[20];
    char E_W[20];
    char* i_status = gps.GET_NMEA(stat, 2);
    char* i_lat = gps.GET_NMEA(lat, 3);
    float latitude = atof(i_lat);  // Convert char* to float
    char* i_N_S = gps.GET_NMEA(N_S, 4);
    char* i_longitude = gps.GET_NMEA(longitude, 5);
    float longitude_float = atof(i_longitude);  // Convert char* to float
    char* i_E_W = gps.GET_NMEA(E_W, 6);


    // Update the struct
    gps_data.data.gps.status = i_status;
    gps_data.data.gps.lat = latitude;
    gps_data.data.gps.N_S = i_N_S;
    gps_data.data.gps.longitude = longitude_float;
    gps_data.data.gps.E_W = i_E_W;
    // --------------------------------------------------------------------------------------------    

    // Store GPS data in SD card file -------------------------------------------------------------
    String fileGPS;
    String data_str_GPS;
    fileGPS = GPS_FILEPATH;
    data_str_GPS =
      String(gps_data.data.gps.hours) + "," + String(gps_data.data.gps.minutes) + "," + String(gps_data.data.gps.seconds) + "," + String(gps_data.data.gps.microseconds) + "," +
      String(gps_data.data.gps.status) + "," + String(gps_data.data.gps.lat) + "," + String(gps_data.data.gps.N_S) + "," + String(gps_data.data.gps.longitude) + "," + String(gps_data.data.gps.E_W);

    data_str_GPS += "\n";
    sd.appendFile(fileGPS.c_str(), data_str_GPS.c_str());


    // Step 1: xQueueReceive from readData queue
    // decode the string and put it in XBeeMsg struct
    XBeeMsg transmission;

      // shove received string into SD card file as it is
      String fileTelemetry = TELEMETRY_FILEPATH;
      String data_str_Telemetry = transmission.text;
      data_str_Telemetry += "\n";
      sd.appendFile(fileTelemetry.c_str(), data_str_Telemetry.c_str());


      // Insert logic to break the gps_data into a string and put into the SensorData file and then put into the SD card file
      TelemetryStruct RxMsg;
      String data_str_RxMsg = transmission.text;

      // Parse the string into the SensorData struct
      int SenderID;
      int RxID;
      float Hrs, Mins, Secs, Microsecs;
      float Altitude;
      char* Status;
      float Latitude;
      char* N_S2;
      float Longitude;
      char* E_W2;

      // Use sscanf to parse the string
      int parsed = sscanf(
        data_str_RxMsg.c_str(),
        "%1[^,],%1[^,],%f,%f,%f,%f,%f,%19[^,],%f,%1[^,],%f,%1[^,]",
        SenderID, RxID, &Hrs, &Mins, &Secs, &Microsecs,
        &Altitude, Status, &Latitude, N_S2, &Longitude, E_W2
    );

      if (parsed != 12) {
        Serial.println("Error: Failed to parse the received string.");
      }
      else {
        // Populate the SensorData struct
        RxMsg.SenderID = SenderID;
        RxMsg.RxID = RxID;
        RxMsg.hours = Hrs;
        RxMsg.minutes = Mins;
        RxMsg.seconds = Secs;
        RxMsg.microseconds = Microsecs;
        RxMsg.altitude = Altitude;
        RxMsg.Status = Status;
        RxMsg.lat = Latitude;
        RxMsg.N_S = N_S2;
        RxMsg.longitude = Longitude;
        RxMsg.E_W = E_W2;

        // Print the parsed data for debugging
        Serial.print("Parsed Data: ");


        // Store the parsed data in the SD card file
        String fileTelemetry = TELEMETRY_FILEPATH;
        String data_str_Telemetry = data_str_RxMsg + "\n";
        sd.appendFile(fileTelemetry.c_str(), data_str_Telemetry.c_str());
      }

      
      // do heading and distance calculations
      float deltaLat;
      float deltaLong;

      if (RxMsg.N_S == gps_data.data.gps.N_S && RxMsg.E_W == gps_data.data.gps.E_W) {
        deltaLat = RxMsg.lat - gps_data.data.gps.lat;
        deltaLong = RxMsg.longitude - gps_data.data.gps.longitude;
      } else if (RxMsg.N_S != gps_data.data.gps.N_S && RxMsg.E_W == gps_data.data.gps.E_W) {
        deltaLat = RxMsg.lat - gps_data.data.gps.lat;
        deltaLong = RxMsg.longitude + gps_data.data.gps.longitude;
      } else if (RxMsg.N_S != gps_data.data.gps.N_S && RxMsg.E_W != gps_data.data.gps.E_W) {
        deltaLat = RxMsg.lat + gps_data.data.gps.lat;
        deltaLong = RxMsg.longitude + gps_data.data.gps.longitude;
      } else if (RxMsg.N_S == gps_data.data.gps.N_S && RxMsg.E_W != gps_data.data.gps.E_W) {
        deltaLat = RxMsg.lat + gps_data.data.gps.lat;
        deltaLong = RxMsg.longitude - gps_data.data.gps.longitude;
      } else {
        deltaLat = 999999;
        deltaLong = 999999;
      }

      float heading = (-atan2(deltaLong, deltaLat) * 180 / M_PI) + 90; // Convert to degrees

      // Normalize to 0-360° range
      if (heading < 0) {
        heading += 360.0;
      }
      if (heading >= 360.0) {
        heading -= 360.0;
      }
      
      // Calculate distance using the Haversine formula
      float distance = sqrt(pow(deltaLong, 2) + pow(deltaLat, 2));

      //Call the display update function to update the display with the new data
      disp.updateHeading(heading); // Update the display with the new heading
      disp.updateRocketLatitude(RxMsg.lat, RxMsg.E_W);
      disp.updateRocketLongitude(RxMsg.longitude, RxMsg.N_S);
      disp.updateUsLatitude(gps_data.data.gps.lat, gps_data.data.gps.N_S);
      disp.updateUsLongitude(gps_data.data.gps.longitude, gps_data.data.gps.E_W);
      disp.updateDistance(distance); // Update the display with the new distance
      disp.updateAltitude(RxMsg.altitude); // Update the display with the new altitude
  
    }
}

void setup() {
  Serial.begin(115200);

  // Initialize the sensors -----------------------------------------------------------------------
  gps.setup();

  disp.setup();

  timer.adjustTime(4, 3, 2025, 18, 41, 00, 000000);

  // Initialize the SD Card -----------------------------------------
  sd.begin();
  sd.writeFile(GPS_FILEPATH, "hours, minutes, seconds, microseconds, status, latitude, North/South, longitude, East/West\n");
  sd.writeFile(TELEMETRY_FILEPATH, "hours, minutes, seconds, microseconds, status, latitude, north/south, longitude, east/west\n");

  // Initial Strings ------------------------------------------------
  String initialGPS = "Initializing GPS";
  String initialTelemetry = "Initializing Telemetry";
  String ifileGPS = GPS_FILEPATH;
  String ifileTelemetry = TELEMETRY_FILEPATH;

  // Starting new iteration -----------------------------------------
  initialGPS += "\n";
  initialTelemetry += "\n";
  sd.appendFile(ifileGPS.c_str(), initialGPS.c_str());
  sd.appendFile(ifileTelemetry.c_str(), initialTelemetry.c_str());

  // Initialize the XBee --------------------------------------------------------------------------
  xbee = new XBeeArduino(serialPort, 115200, XBEE_STANDARD, OnReceiveCallback, OnSendCallback);
  mySerial.begin(115200, SERIAL_8N1, -1, -1);

  if (!xbee->begin()) {
    Serial.println("Failed to initialize XBee");
    while (1)
      ; // Halt the program on failure

  }

  // Initialize Queues ----------------------------------------------
  XBee_Qu = xQueueCreate(10, sizeof(XBeeMsg)); // Changed to XBeeMsg type

  // RTOS Initialization --------------------------------------------------------------------------
  xTaskCreatePinnedToCore(
    MegaTask,
    "Mega Task",
    16384,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    XBeeTask,
    "XBee Task",
    16384,
    NULL,
    2,
    NULL,
    app_cpu
  );
}

void loop() {

}