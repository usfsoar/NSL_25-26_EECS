#include <Adafruit_NeoPixel.h>

// --- LED configuration ---
#define LED_PIN 21
#define NUM_LEDS 1

Adafruit_NeoPixel onboardLed = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Initializing...");

  // --- Initialize NeoPixel ---
  onboardLed.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  onboardLed.setBrightness(50);  // Set BRIGHTNESS (optional, 0-255)
  onboardLed.clear();            // Set all pixel colors to 'off'
  onboardLed.show();             // Send the clear command to the pixel

  Serial.println("Onboard NeoPixel Initialized.");
}

void loop() {
  // Turn LED Green
  onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 50)); // Green
  onboardLed.show();
  delay(1000); // ON for 1 second

  // Turn LED Off
  onboardLed.clear();
  onboardLed.show();
  delay(1000); // OFF for 1 second
}
