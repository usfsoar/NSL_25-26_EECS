#ifndef SOAR_BNO085_H
#define SOAR_BNO085_H

// For SPI mode, we also need a RESET
//#define BNO08X_RESET 5
// but not for I2C or UART
#define BNO08X_RESET -1

#include <Adafruit_BNO08x.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

class SOAR_BNO085 {
    public:
        typedef struct {
            float x, y, z;
        } Vector3D_t;

        // quat with respect to gravity and north
        typedef struct {
            float w, x, y, z;
            float accuracy;
        } Orientation_t;

        typedef struct {
            Vector3D_t acceleration;
            Vector3D_t linearAcceleration;
            Vector3D_t gravity;
            Vector3D_t gyroscope;
            Vector3D_t magneticField;
            Orientation_t orientation;
        } AllSensorData_t;

        AllSensorData_t sensorData;

        SOAR_BNO085();  // Constructor
        void update();  // Polls sensor for new data, should be called in a loop
        void showState();  // Polls sensor for new data, should be called in a loop
        void begin();

        Adafruit_BNO08x bno08x;
        sh2_SensorValue_t sensorValue;

        void setReports();
        void processSensorEvent();
};

#endif // SOAR_BNO085_H
