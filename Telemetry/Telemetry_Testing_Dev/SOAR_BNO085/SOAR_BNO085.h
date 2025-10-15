/*
 * SOAR_BNO085.h
 *
 * Header file for the SOAR_BNO085 class. This class encapsulates all functionality
 * for the BNO085 IMU, providing a clean interface for initialization,
 * data polling, and retrieval within a FreeRTOS environment.
 */

#ifndef SOAR_BNO085_H
#define SOAR_BNO085_H

#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"

class SOAR_BNO085 {
public:
    // --- Public Data Structures ---
    typedef struct {
        float x, y, z;
    } Vector3D_t;

    typedef struct {
        float x, y, z; // X: Roll, Y: Pitch, Z: Yaw
        float accuracy;
    } Orientation_t;

    typedef struct {
        Vector3D_t acceleration;
        Vector3D_t linearAcceleration;
        Vector3D_t gravity;
        Vector3D_t gyroscope;
        Vector3D_t magneticField;
        Vector3D_t velocity;
        Orientation_t orientation;
    } AllSensorData_t;

    // --- Public Member Functions ---
    SOAR_BNO085();  // Constructor
    ~SOAR_BNO085(); // Destructor
    bool begin();   // Initializes the sensor
    void update();  // Polls sensor for new data, should be called in a loop
    
    AllSensorData_t getAllData(); // Thread-safe method to get a copy of all data

private:
    // --- Private Member Variables ---
    Adafruit_BNO08x bno08x;
    sh2_SensorValue_t sensorValue;
    AllSensorData_t sensorData;
    SemaphoreHandle_t dataMutex;
    unsigned long lastVelocityUpdate;

    // --- Private Helper Functions ---
    void processSensorEvent();
    void updateVelocity();
    void updateOrientation();
};

#endif // SOAR_BNO085_H
