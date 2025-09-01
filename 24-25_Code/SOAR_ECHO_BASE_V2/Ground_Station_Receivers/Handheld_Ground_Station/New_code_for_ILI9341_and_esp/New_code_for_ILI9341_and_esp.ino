#include <Arduino.h>
#include <string.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pins for ESP-WROOM with Adafruit ILI9341 display
#define TFT_MISO 19
#define TFT_LED 5
#define TFT_SCK 18
#define TFT_MOSI 23
#define TFT_DC 2
#define TFT_RESET 4
#define TFT_CS 15

// Initialize display with ESP-WROOM pins
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RESET, TFT_MISO);

// Check if the board has a single or dual core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Define message types for the unified queue
#define MSG_BATTERY 1
#define MSG_HEADING 2
#define MSG_ALTITUDE 3
#define MSG_DISTANCE 4
#define MSG_ROCKET_LAT 5
#define MSG_ROCKET_LONG 6
#define MSG_US_LAT 7
#define MSG_US_LONG 8

// Define a unified message structure
typedef struct {
  uint8_t msgType;      // Type of message
  union {
    int intValue;       // For battery, altitude, distance
    float floatValue;   // For heading, latitude, longitude
  };
} Message_t;

// Single queue handle
QueueHandle_t dataQueue;

// Constants for drawing
#define BACKGROUND_COLOR ILI9341_BLACK
#define TEXT_COLOR ILI9341_WHITE
#define LINE_COLOR ILI9341_WHITE

// Global variables for layout
int screenWidth, screenHeight;
int marginX = 5, marginY = 5;
int formWidth, formHeight;
int sectionSpacing = 5;
int totalSections = 4;
int totalSpacing;
int headerHeight, fieldHeight, tableHeight;

// Current values for display (to avoid redrawing unchanged values)
int currentBattery = 0;
float currentHeading = 0.0;
int currentAltitude = 0;
int currentDistance = 0;
float currentRocketLat = 0.0;
float currentRocketLong = 0.0;
float currentUsLat = 0.0;
float currentUsLong = 0.0;

// Function to normalize input (remove spaces and make lowercase)
String normalizeInput(String input) {
  input.trim();              // Remove leading/trailing spaces
  input.toLowerCase();       // Convert to lowercase
  input.replace(" ", "");    // Remove all spaces
  return input;
}

// Forward declarations of LCD drawing functions
void drawRocketTrackingForm();
void updateBatteryPercentage(int percentage);
void updateHeading(float heading);
void updateAltitude(int altitude);
void updateDistance(int distance);
void updateRocketLatitude(float latitude);
void updateRocketLongitude(float longitude);
void updateUsLatitude(float latitude);
void updateUsLongitude(float longitude);

// Function to read serial input and parse values
void readSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n'); // Read input line
    input.trim();                                // Remove spaces

    // Extract the keyword part and normalize it
    int colonIndex = input.indexOf(':');
    if (colonIndex == -1) return;               // Ignore if no colon is found

    String keyPart = input.substring(0, colonIndex);
    keyPart = normalizeInput(keyPart);          // Normalize keyword

    String numStr = input.substring(colonIndex + 1);
    numStr.trim(); // Remove spaces

    // Create a message to send to the queue
    Message_t msg;

    // Parse the value based on the key
    if (keyPart == "battery") {
      msg.msgType = MSG_BATTERY;
      msg.intValue = numStr.toInt();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "headingtorocket") {
      msg.msgType = MSG_HEADING;
      msg.floatValue = numStr.toFloat();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "rocketaltitude") {
      msg.msgType = MSG_ALTITUDE;
      msg.intValue = numStr.toInt();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "distancetorocket") {
      msg.msgType = MSG_DISTANCE;
      msg.intValue = numStr.toInt();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "rocketlatitude") {
      msg.msgType = MSG_ROCKET_LAT;
      msg.floatValue = numStr.toFloat();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "rocketlongitude") {
      msg.msgType = MSG_ROCKET_LONG;
      msg.floatValue = numStr.toFloat();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "uslatitude") {
      msg.msgType = MSG_US_LAT;
      msg.floatValue = numStr.toFloat();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
    else if (keyPart == "uslongitude") {
      msg.msgType = MSG_US_LONG;
      msg.floatValue = numStr.toFloat();
      xQueueSend(dataQueue, &msg, portMAX_DELAY);
    }
  }
}

// Task to read serial input
void serialInputTask(void* parameters) {
  while (1) {
    readSerialInput();
    vTaskDelay(50 / portTICK_PERIOD_MS); // Reduced delay for faster response
  }
}

// Task to process messages and update display
void processDataTask(void* parameters) {
  Message_t msg;

  while (1) {
    if (xQueueReceive(dataQueue, &msg, portMAX_DELAY) == pdTRUE) {
      // Process the message based on its type
      switch (msg.msgType) {
      case MSG_BATTERY:
        if (currentBattery != msg.intValue) {
          currentBattery = msg.intValue;
          updateBatteryPercentage(msg.intValue);
          Serial.print("Battery: ");
          Serial.print(msg.intValue);
          Serial.println("%");
        }
        break;

      case MSG_HEADING:
        if (currentHeading != msg.floatValue) {
          currentHeading = msg.floatValue;
          updateHeading(msg.floatValue);
          Serial.print("Heading to Rocket: ");
          Serial.print(msg.floatValue);
          Serial.println("°");
        }
        break;

      case MSG_ALTITUDE:
        if (currentAltitude != msg.intValue) {
          currentAltitude = msg.intValue;
          updateAltitude(msg.intValue);
          Serial.print("Rocket Altitude: ");
          Serial.print(msg.intValue);
          Serial.println("m");
        }
        break;

      case MSG_DISTANCE:
        if (currentDistance != msg.intValue) {
          currentDistance = msg.intValue;
          updateDistance(msg.intValue);
          Serial.print("Distance to Rocket: ");
          Serial.print(msg.intValue);
          Serial.println("m");
        }
        break;

      case MSG_ROCKET_LAT:
        if (currentRocketLat != msg.floatValue) {
          currentRocketLat = msg.floatValue;
          updateRocketLatitude(msg.floatValue);
          Serial.print("Rocket Latitude: ");
          Serial.print(msg.floatValue);
          Serial.println("°");
        }
        break;

      case MSG_ROCKET_LONG:
        if (currentRocketLong != msg.floatValue) {
          currentRocketLong = msg.floatValue;
          updateRocketLongitude(msg.floatValue);
          Serial.print("Rocket Longitude: ");
          Serial.print(msg.floatValue);
          Serial.println("°");
        }
        break;

      case MSG_US_LAT:
        if (currentUsLat != msg.floatValue) {
          currentUsLat = msg.floatValue;
          updateUsLatitude(msg.floatValue);
          Serial.print("Our Latitude: ");
          Serial.print(msg.floatValue);
          Serial.println("°");
        }
        break;

      case MSG_US_LONG:
        if (currentUsLong != msg.floatValue) {
          currentUsLong = msg.floatValue;
          updateUsLongitude(msg.floatValue);
          Serial.print("Our Longitude: ");
          Serial.print(msg.floatValue);
          Serial.println("°");
        }
        break;
      }
    }
  }
}

// Function to draw the rocket tracking form in vertical orientation
void drawRocketTrackingForm() {
  // Get screen dimensions
  screenWidth = tft.width();
  screenHeight = tft.height();

  // Calculate usable area
  formWidth = screenWidth - (2 * marginX);
  formHeight = screenHeight - (2 * marginY);

  // Calculate section heights to fill the entire screen
  totalSpacing = sectionSpacing * (totalSections - 1);

  // Adjusted proportions
  headerHeight = (formHeight - totalSpacing) * 0.15; // 2 headers = 30% total
  fieldHeight = (formHeight - totalSpacing) * 0.175; // 17.5%
  tableHeight = (formHeight - totalSpacing) * 0.35; // 35%

  // Adjust text size based on screen dimensions
  int bigTextSize = max(1, min(3, screenWidth / 130));
  int smallTextSize = max(1, bigTextSize - 1);

  // Clear the screen
  tft.fillScreen(BACKGROUND_COLOR);

  // Draw the main form border
  tft.drawRect(marginX, marginY, formWidth, formHeight, LINE_COLOR);

  // Set text properties
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(smallTextSize);

  // Current Y position tracker
  int currentY = marginY;

  // 1. HEADER SECTION - Battery and Heading
  // Draw header box
  tft.drawRect(marginX, currentY, formWidth, headerHeight * 2, LINE_COLOR);
  tft.drawFastHLine(marginX, currentY + headerHeight, formWidth, LINE_COLOR); // Divider

  // Battery label
  tft.setCursor(marginX + 10, currentY + (headerHeight / 2) - 10);
  tft.print("Battery %");
  tft.drawFastHLine(marginX + 10, currentY + headerHeight - 10, formWidth - 20, LINE_COLOR);

  // Heading label
  tft.setCursor(marginX + 10, currentY + headerHeight + (headerHeight / 2) - 10);
  tft.print("Heading to rocket");  // Updated label to indicate degrees
  tft.drawFastHLine(marginX + 10, currentY + (headerHeight * 2) - 10, formWidth - 20, LINE_COLOR);

  // Update current Y position
  currentY += headerHeight * 2 + sectionSpacing;

  // 2. ALTITUDE SECTION
  tft.drawRect(marginX, currentY, formWidth, fieldHeight, LINE_COLOR);
  tft.setCursor(marginX + 10, currentY + (fieldHeight / 2) - 10);
  tft.print("Rocket altitude (m)");
  tft.drawFastHLine(marginX + 10, currentY + fieldHeight - 10, formWidth - 20, LINE_COLOR);

  // Update current Y position
  currentY += fieldHeight + sectionSpacing;

  // 3. DISTANCE SECTION
  tft.drawRect(marginX, currentY, formWidth, fieldHeight, LINE_COLOR);
  tft.setCursor(marginX + 10, currentY + (fieldHeight / 2) - 10);
  tft.print("Distance to rocket (m)");
  tft.drawFastHLine(marginX + 10, currentY + fieldHeight - 10, formWidth - 20, LINE_COLOR);

  // Update current Y position
  currentY += fieldHeight + sectionSpacing;

  // 4. COORDINATES TABLE (3x3 layout)
  int tableStartY = currentY;
  int tableWidth = formWidth;
  int rowHeight = tableHeight / 3; // Three rows

  // Calculate column widths for 3 columns
  int labelColWidth = tableWidth / 3;
  int dataColWidth = (tableWidth - labelColWidth) / 2;

  // Draw table outer border
  tft.drawRect(marginX, tableStartY, tableWidth, tableHeight, LINE_COLOR);

  // Draw horizontal dividers for rows
  for (int i = 1; i < 3; i++) {
    tft.drawFastHLine(marginX, tableStartY + (rowHeight * i), tableWidth, LINE_COLOR);
  }

  // Draw vertical dividers for columns
  tft.drawFastVLine(marginX + labelColWidth, tableStartY, tableHeight, LINE_COLOR); // After labels
  tft.drawFastVLine(marginX + labelColWidth + dataColWidth, tableStartY, tableHeight, LINE_COLOR); // Between data

  // Table headers (top row)
  // Column 2 header: "Rocket"
  int rocketX = marginX + labelColWidth + (dataColWidth / 2) - 20;
  tft.setCursor(rocketX, tableStartY + (rowHeight / 2) - 5);
  tft.print("Rocket");

  // Column 3 header: "Us"
  int usX = marginX + labelColWidth + dataColWidth + (dataColWidth / 2) - 10;
  tft.setCursor(usX, tableStartY + (rowHeight / 2) - 5);
  tft.print("Us");

  // Row labels (first column, rows 2 and 3)
  int labelX = marginX + 10;

  // Row 2: "latitude"
  int latitudeY = tableStartY + rowHeight + (rowHeight / 2) - 5;
  tft.setCursor(labelX, latitudeY);
  tft.print("latitude");

  // Row 3: "longitude"
  int longitudeY = tableStartY + (rowHeight * 2) + (rowHeight / 2) - 5;
  tft.setCursor(labelX, longitudeY);
  tft.print("longitude");
}

// Optimized update functions that only redraw when values change
void updateBatteryPercentage(int percentage) {
  int valueX = marginX + 100; // Position after the label
  int valueY = marginY + (headerHeight / 2) - 10;

  // Clear previous value area
  tft.fillRect(valueX, valueY, 70, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[10];
  sprintf(buffer, "%d%%", percentage);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateHeading(float heading) {
  int valueX = marginX + 170; // Position after the label
  int valueY = marginY + headerHeight + (headerHeight / 2) - 10;

  // Clear previous value area
  tft.fillRect(valueX, valueY, 100, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  dtostrf(heading, 6, 2, buffer); // Convert float to string with 2 decimal places
  strcat(buffer, "");            // Add the degree symbol
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateAltitude(int altitude) {
  int valueX = marginX + 170; // Position after the label
  int valueY = marginY + (headerHeight * 2) + sectionSpacing + (fieldHeight / 2) - 10;

  // Clear previous value area
  tft.fillRect(valueX, valueY, 100, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  sprintf(buffer, "%d m", altitude);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateDistance(int distance) {
  int valueX = marginX + 170; // Position after the label
  int valueY = marginY + (headerHeight * 2) + sectionSpacing + fieldHeight + sectionSpacing + (fieldHeight / 2) - 10;

  // Clear previous value area
  tft.fillRect(valueX, valueY, 100, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  sprintf(buffer, "%d m", distance);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateRocketLatitude(float latitude) {
  int tableStartY = marginY + (headerHeight * 2) + (sectionSpacing * 3) + (fieldHeight * 2);
  int rowHeight = tableHeight / 3;
  int labelColWidth = formWidth / 3;
  int dataColWidth = (formWidth - labelColWidth) / 2;

  int valueX = marginX + labelColWidth + 10;
  int valueY = tableStartY + rowHeight + (rowHeight / 2) - 5;

  // Clear previous value area
  tft.fillRect(valueX, valueY, dataColWidth - 20, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  dtostrf(latitude, 8, 4, buffer); // Convert float to string with 4 decimal places
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateUsLatitude(float latitude) {
  int tableStartY = marginY + (headerHeight * 2) + (sectionSpacing * 3) + (fieldHeight * 2);
  int rowHeight = tableHeight / 3;
  int labelColWidth = formWidth / 3;
  int dataColWidth = (formWidth - labelColWidth) / 2;

  int valueX = marginX + labelColWidth + dataColWidth + 10;
  int valueY = tableStartY + rowHeight + (rowHeight / 2) - 5;

  // Clear previous value area
  tft.fillRect(valueX, valueY, dataColWidth - 20, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  dtostrf(latitude, 8, 4, buffer); // Convert float to string with 4 decimal places
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateRocketLongitude(float longitude) {
  int tableStartY = marginY + (headerHeight * 2) + (sectionSpacing * 3) + (fieldHeight * 2);
  int rowHeight = tableHeight / 3;
  int labelColWidth = formWidth / 3;
  int dataColWidth = (formWidth - labelColWidth) / 2;

  int valueX = marginX + labelColWidth + 10;
  int valueY = tableStartY + (rowHeight * 2) + (rowHeight / 2) - 5;

  // Clear previous value area
  tft.fillRect(valueX, valueY, dataColWidth - 20, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  dtostrf(longitude, 8, 4, buffer); // Convert float to string with 4 decimal places
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void updateUsLongitude(float longitude) {
  int tableStartY = marginY + (headerHeight * 2) + (sectionSpacing * 3) + (fieldHeight * 2);
  int rowHeight = tableHeight / 3;
  int labelColWidth = formWidth / 3;
  int dataColWidth = (formWidth - labelColWidth) / 2;

  int valueX = marginX + labelColWidth + dataColWidth + 10;
  int valueY = tableStartY + (rowHeight * 2) + (rowHeight / 2) - 5;

  // Clear previous value area
  tft.fillRect(valueX, valueY, dataColWidth - 20, 20, BACKGROUND_COLOR);

  // Display new value
  char buffer[15];
  dtostrf(longitude, 8, 4, buffer); // Convert float to string with 4 decimal places
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

// Function to send an initial data set to the queue
void setInitialValues() {
  Message_t msg;

  // Battery
  msg.msgType = MSG_BATTERY;
  msg.intValue = 0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Heading
  msg.msgType = MSG_HEADING;
  msg.floatValue = 0.0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Altitude
  msg.msgType = MSG_ALTITUDE;
  msg.intValue = 0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Distance
  msg.msgType = MSG_DISTANCE;
  msg.intValue = 0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Rocket Latitude
  msg.msgType = MSG_ROCKET_LAT;
  msg.floatValue = 0.0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Rocket Longitude
  msg.msgType = MSG_ROCKET_LONG;
  msg.floatValue = 0.0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Us Latitude
  msg.msgType = MSG_US_LAT;
  msg.floatValue = 0.0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);

  // Us Longitude
  msg.msgType = MSG_US_LONG;
  msg.floatValue = 0.0;
  xQueueSend(dataQueue, &msg, portMAX_DELAY);
}

// Setup function
void setup() {
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Small delay to stabilize Serial

  // Initialize display
  tft.begin();
  tft.setRotation(0); // Portrait mode
  tft.fillScreen(BACKGROUND_COLOR);

  // LED backlight control for ESP-WROOM
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); // Turn on backlight

  // Draw the UI form
  drawRocketTrackingForm();

  Serial.println("\n--- FreeRTOS Rocket Tracking System ---");
  Serial.println("Enter values in the format: 'parameter: value'");
  Serial.println("Example: 'battery: 75' or 'headingtorocket: 45.5'");

  // Create the unified queue (can hold up to 10 messages)
  dataQueue = xQueueCreate(10, sizeof(Message_t));

  // Create tasks
  xTaskCreatePinnedToCore(serialInputTask, "SerialInput", 2048, NULL, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(processDataTask, "ProcessData", 3072, NULL, 2, NULL, app_cpu);

  // Initialize with default values
  setInitialValues();
}

void loop() {
  // Unused as FreeRTOS tasks handle everything
  vTaskDelete(NULL); // Delete the loop task
}