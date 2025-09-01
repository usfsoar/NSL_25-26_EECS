// filepath: c:\Users\pav93\Downloads\SOAR_Repositories\NSL_24-25_EECS\SOAR_ECHO_BASE_V2\Ground_Station_Receivers\v1_GS_Handheld\v1_SOAR_RTOS_DISPLAY.h
#ifndef SOAR_RTOS_DISPLAY_H
#define SOAR_RTOS_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Constants for drawing
#define BACKGROUND_COLOR ILI9341_BLACK
#define TEXT_COLOR ILI9341_WHITE
#define LINE_COLOR ILI9341_WHITE

class SOAR_RTOS_DISPLAY {
public:
  SOAR_RTOS_DISPLAY(Adafruit_ILI9341& tft); // Constructor

  void setup();
  void drawRocketTrackingForm();
  void updateBatteryPercentage(int percentage);
  void updateHeading(float heading);
  void updateAltitude(int altitude);
  void updateDistance(int distance);
  void updateRocketLatitude(float latitude, char* E_W);
  void updateRocketLongitude(float longitude, char* N_S);
  void updateUsLatitude(float latitude, char* E_W);
  void updateUsLongitude(float longitude, char* N_S);
  // void setInitialValues();

private:
  Adafruit_ILI9341& tft;

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

  // Helper function to normalize input
  String normalizeInput(String input);
};

#endif // SOAR_RTOS_DISPLAY_H