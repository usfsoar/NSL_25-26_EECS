#include "v1_SOAR_RTOS_DISPLAY.h"


SOAR_RTOS_DISPLAY::SOAR_RTOS_DISPLAY(Adafruit_ILI9341& tft) : tft(tft) {
  // Initialize screen dimensions
  screenWidth = tft.width();
  screenHeight = tft.height();
  
  // Calculate form dimensions
  formWidth = screenWidth - (2 * marginX);
  formHeight = screenHeight - (2 * marginY);
  
  // Calculate layout dimensions
  totalSpacing = sectionSpacing * (totalSections - 1);
  headerHeight = (formHeight - totalSpacing) / totalSections;
  fieldHeight = headerHeight;
  tableHeight = headerHeight * 2;
  
  // Initialize display values to 0
  // setInitialValues();
}

typedef struct {
  uint8_t msgType;      // Type of message
  union {
    int intValue;       // For battery, altitude, distance
    float floatValue;   // For heading, latitude, longitude
  };
} Display_Msg;

// Constants for drawing
#define BACKGROUND_COLOR ILI9341_BLACK
#define TEXT_COLOR ILI9341_WHITE
#define LINE_COLOR ILI9341_WHITE
#define TFT_LED 1

void SOAR_RTOS_DISPLAY::setup() {
  tft.begin();
  tft.setRotation(0); // Portrait mode
  tft.fillScreen(BACKGROUND_COLOR);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH); // Turn on backlight

  drawRocketTrackingForm();
}

// Function to normalize input (remove spaces and make lowercase)
String normalizeInput(String input) {
  input.trim();              // Remove leading/trailing spaces
  input.toLowerCase();       // Convert to lowercase
  input.replace(" ", "");    // Remove all spaces
  return input;
}

// Function to draw the rocket tracking form in vertical orientation
void SOAR_RTOS_DISPLAY::drawRocketTrackingForm() {
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
void SOAR_RTOS_DISPLAY::updateBatteryPercentage(int percentage) {
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

void SOAR_RTOS_DISPLAY::updateHeading(float heading) {
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

void SOAR_RTOS_DISPLAY::updateAltitude(int altitude) {
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

void SOAR_RTOS_DISPLAY::updateDistance(int distance) {
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

void SOAR_RTOS_DISPLAY::updateRocketLatitude(float latitude, char* E_W) {
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
  strcat(buffer, "° ");           // Add degree symbol and space
  strcat(buffer, E_W);            // Add E/W indicator
    

  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void SOAR_RTOS_DISPLAY::updateUsLatitude(float latitude, char* E_W) {
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
  strcat(buffer, "° ");           // Add degree symbol and space
  strcat(buffer, E_W);            // Add E/W indicator

  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void SOAR_RTOS_DISPLAY::updateRocketLongitude(float longitude, char* N_S) {
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
  strcat(buffer, "° ");           // Add degree symbol and space
  strcat(buffer, N_S);            // Add N/S indicator

  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}

void SOAR_RTOS_DISPLAY::updateUsLongitude(float longitude, char* N_S) {
  int tableStartY = marginY + (headerHeight * 2) + (sectionSpacing * 3) + (fieldHeight * 2);
  int rowHeight = tableHeight / 3;
  int labelColWidth = formWidth / 3;
  int dataColWidth = (formWidth - labelColWidth) / 2;

  int valueX = marginX + labelColWidth + dataColWidth + 10;
  int valueY = tableStartY + (rowHeight * 2) + (rowHeight / 2) - 5;

  // Clear previous value area
  char buffer[15];
  tft.fillRect(valueX, valueY, dataColWidth - 20, 20, BACKGROUND_COLOR);
  strcat(buffer, "° ");           // Add degree symbol and space
  strcat(buffer, N_S);            // Add N/S indicator

  // Display new value
  dtostrf(longitude, 8, 4, buffer); // Convert float to string with 4 decimal places
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(valueX, valueY);
  tft.print(buffer);
}











