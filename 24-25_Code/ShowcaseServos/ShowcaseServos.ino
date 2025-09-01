#include "SOAR_PAYLOAD_SERVO.h"
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <queue>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
SOAR_PAYLOAD_SERVO p_serv;
std::queue<String> messageQueue;

enum AnimationMode {
  ALL_TOGETHER,
  SPIRAL_OPEN_CLOSE,
  WAVE,
  ALTERNATING_PAIRS,
  RANDOM_PATTERN,
  PENGUIN_WADDLE,
  UP_FACING_DOOR,
  THEY_DONT_LOVE_YOU,
  NUM_MODES = 8
};

int currentMode = 0;
unsigned long modeStartTime = 0;
const unsigned long modeDuration = 15000;  // 15 seconds per mode

// IMU simulation variables (replace with actual IMU code)
float gravity[3];  // Start with Z-axis up
unsigned long lastImuUpdate = 0;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      messageQueue.push(String(value.c_str()));  // Enqueue the incoming message
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Initialize the Wire library with specified SDA and SCL pins
  Wire.begin(1, 2);  // SDA on pin 1, SCL on pin 2

  p_serv.initialize(pwm);
  Serial.println("Servo Controller Initialized");
  modeStartTime = millis();

  BLEDevice::init("SOAR_SentryTurret");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  pCharacteristic = pService->createCharacteristic(
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
    BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  // Start advertising
  pServer->getAdvertising()->start();
}

void loop() {
  // Disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // Give the Bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // Restart advertising
    Serial.println("Advertising started");
    oldDeviceConnected = deviceConnected;
  }

  // // Connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  while (!messageQueue.empty()) {
    String msg = messageQueue.front();
    messageQueue.pop();
    if (msg.startsWith("AT:")) {
      int num = msg.substring(3).toInt();
      Serial.print("AT command received with value: ");
      Serial.println(num);
      armsServo.autoMove(num);
    } else if (msg.startsWith("TT:")) {
      int num = msg.substring(3).toInt();
      Serial.print("TT command received with value: ");
      Serial.println(num);
      torsoServo.autoMove(num);
    } else if (msg.startsWith("ZT:")) {
      int num = msg.substring(3).toInt();
      Serial.print("ZT command received with value: ");
      Serial.println(num);
      if (num == 1) {
        Serial.println("Taser ON");
        taser.fire();
      }
      if (num == 0) {
        Serial.println("Taser OFF");
        taser.stop();
      }
    }
  }

  unsigned long currentTime = millis();

  // Update simulated IMU data (replace with actual IMU readings)
  if (currentTime - lastImuUpdate > 1000) {
    lastImuUpdate = currentTime;
    // Cycle through different orientations for demo
    static int imuState = 0;
    imuState = (imuState + 1) % 4;
    switch (imuState) {
      case 0:
        gravity[0] = 1;
        gravity[2] = 0;
        break;  // X up
      case 1:
        gravity[0] = 0;
        gravity[2] = 1;
        break;  // Z up
      case 2:
        gravity[0] = -1;
        gravity[2] = 0;
        break;  // -X up
      case 3:
        gravity[0] = 0;
        gravity[2] = -1;
        break;  // -Z up
    }
  }

  // Switch mode after duration elapses
  if (currentTime - modeStartTime >= modeDuration) {
    modeStartTime = currentTime;
    currentMode = (currentMode + 1) % NUM_MODES;
    Serial.print("Switching to mode: ");
    Serial.println(currentMode);
  }

  // Continuous animation playback
  switch (currentMode) {
    case ALL_TOGETHER:
      allTogetherAnimation();
      break;
    case SPIRAL_OPEN_CLOSE:
      spiralAnimation();
      break;
    case WAVE:
      waveAnimation();
      break;
    case ALTERNATING_PAIRS:
      alternatingPairsAnimation();
      break;
    case RANDOM_PATTERN:
      randomPatternAnimation();
      break;
    case PENGUIN_WADDLE:
      penguinWaddleAnimation();
      break;
    case UP_FACING_DOOR:
      upFacingDoorAnimation();
      break;
    case THEY_DONT_LOVE_YOU:
      theyDontLoveYouLikeILoveYou();
      break;
  }
}

// Animation 1: All doors open and close together
void allTogetherAnimation() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 30;
  static int currentAngle = 0;
  static bool opening = true;

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    if (opening) {
      currentAngle++;
      if (currentAngle >= 44) opening = false;
    } else {
      currentAngle--;
      if (currentAngle <= 0) opening = true;
    }

    p_serv.setServoAngle(pwm, 0, currentAngle);
    p_serv.setServoAngle(pwm, 4, currentAngle);
    p_serv.setServoAngle(pwm, 8, currentAngle);
    p_serv.setServoAngle(pwm, 12, currentAngle);
  }
}

// Animation 2: Spiral opening and closing
void spiralAnimation() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 100;
  static int currentStep = 0;
  static int activeDoor = 0;
  static bool opening = true;
  const int doors[4] = { 0, 4, 8, 12 };

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    Serial.println("Playing: Spiral");

    // Open in sequence
    openDoorWithDelay(0, 5);
    closeDoorWithDelay(12, 5);
    openDoorWithDelay(4, 5);
    closeDoorWithDelay(0, 5);
    openDoorWithDelay(8, 5);
    closeDoorWithDelay(4, 5);
    openDoorWithDelay(12, 5);
    closeDoorWithDelay(8, 5);
  }
}

// Animation 3: Wave pattern
// Animation 3: Wave pattern
void waveAnimation() {
  Serial.println("Playing: Wave");

  // Two cycles of wave
  for (int i = 0; i < 5; i++) {
    // Open doors in sequence
    for (int channel = 0; channel <= 12; channel += 4) {
      openDoorWithDelay(channel, 10);
    }

    // Close doors in sequence
    for (int channel = 0; channel <= 12; channel += 4) {
      closeDoorWithDelay(channel, 10);
    }
  }
}

// Animation 4: Alternating pairs
void alternatingPairsAnimation() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 50;
  static int currentPair = 0;
  static int currentAngle = 0;
  static bool opening = true;
  const int pairs[2][2] = { { 0, 8 }, { 4, 12 } };

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    if (opening) {
      currentAngle++;
      if (currentAngle >= 44) {
        opening = false;
      }
    } else {
      currentAngle--;
      if (currentAngle <= 0) {
        opening = true;
        currentPair = (currentPair + 1) % 2;
      }
    }

    // Open current pair
    p_serv.setServoAngle(pwm, pairs[currentPair][0], currentAngle);
    p_serv.setServoAngle(pwm, pairs[currentPair][1], currentAngle);

    // Close other pair
    int otherPair = (currentPair + 1) % 2;
    p_serv.setServoAngle(pwm, pairs[otherPair][0], 0);
    p_serv.setServoAngle(pwm, pairs[otherPair][1], 0);
  }
}

// Animation 5: Random pattern
void randomPatternAnimation() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 500;
  const int doors[4] = { 0, 4, 8, 12 };

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    // Randomly set each door
    for (int i = 0; i < 4; i++) {
      if (random(100) > 70) {  // 30% chance to change
        int angle = random(0, 45);
        p_serv.setServoAngle(pwm, doors[i], angle);
      }
    }
  }
}

// Animation 6: Penguin waddle
void penguinWaddleAnimation() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 300;
  static int currentWaddle = 0;
  const int doors[2] = { 0, 8 };  // Opposite doors

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    currentWaddle = (currentWaddle + 1) % 2;

    // Open current door, close other
    p_serv.setServoAngle(pwm, doors[currentWaddle], 44);
    p_serv.setServoAngle(pwm, doors[(currentWaddle + 1) % 2], 0);
  }
}


// Animation 7: Open upward-facing door
void upFacingDoorAnimation() {
  // Use actual IMU data (gravity vector)
  p_serv.servoLogic(pwm, gravity);
}

// Animation 6: Penguin waddle - Note: This seems to be a duplicate case number and name as Animation 6.
// Assuming this is intended to be a different animation, consider renaming the enum value.
// However, for the purpose of modifying the existing code structure, I will keep the function name as is.
void theyDontLoveYouLikeILoveYou() {
  static unsigned long lastUpdate = 0;
  const int updateInterval = 300;
  static int currentWaddle = 0;
  const int doors[2] = { 0, 8 };  // Opposite doors

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    p_serv.setServoAngle(pwm, 0, 0);
    p_serv.setServoAngle(pwm, 4, 0);
    p_serv.setServoAngle(pwm, 8, 0);
    p_serv.setServoAngle(pwm, 12, 0);
    delay(40);
    for (int i = 0; i < 1; i++) {
      p_serv.setServoAngle(pwm, 0, 0);
      p_serv.setServoAngle(pwm, 4, 44);
      p_serv.setServoAngle(pwm, 8, 0);
      p_serv.setServoAngle(pwm, 12, 0);
      delay(50);
      // Open current door, close other
      for (int i = 0; i < 7; i++) {
        currentWaddle = (currentWaddle + 1) % 2;
        p_serv.setServoAngle(pwm, doors[currentWaddle], 44);
        p_serv.setServoAngle(pwm, doors[(currentWaddle + 1) % 2], 0);
        delay(50);
      }
    }
    for (int i = 0; i < 1; i++) {
      p_serv.setServoAngle(pwm, 0, 0);
      p_serv.setServoAngle(pwm, 4, 44);
      p_serv.setServoAngle(pwm, 8, 0);
      p_serv.setServoAngle(pwm, 12, 0);
      delay(300);
      // Open current door, close other
      for (int i = 0; i < 7; i++) {
        currentWaddle = (currentWaddle + 1) % 2;
        p_serv.setServoAngle(pwm, doors[currentWaddle], 44);
        p_serv.setServoAngle(pwm, doors[(currentWaddle + 1) % 2], 0);
        delay(50);
      }
    }
  }
}

// Helper function to open a door with smooth animation
void openDoorWithDelay(int channel, int stepDelay) {
  for (int angle = 0; angle <= 44; angle++) {
    p_serv.setServoAngle(pwm, channel, angle);
    delay(stepDelay);
  }
}

// Helper function to close a door with smooth animation
void closeDoorWithDelay(int channel, int stepDelay) {
  for (int angle = 44; angle >= 0; angle--) {
    p_serv.setServoAngle(pwm, channel, angle);
    delay(stepDelay);
  }
}
