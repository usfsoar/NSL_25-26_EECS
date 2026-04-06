#include <stdio.h>
#include <ESP32Servo.h>
#include "antenna_actuation.h"
#include "pcg.h"

#define SERVO_PIN D4
Servo antenna_servo;

pcg32_random_t my_rng;

double init_lat, init_long;
double new_long, new_lat;
double delta_long, delta_lat;
double phi, phi_deg, elevation_angle;
double altitude; /* meters */

void randomWalk(pcg32_random_t *rng, double *latitude, double *longitude) {
    double sigma_x = 0.01;
    double sigma_y = 0.01;
    *longitude += sigma_x * pcg32_normal_r(rng);
    *latitude += sigma_y * pcg32_normal_r(rng);
    return;
}

void setup(void)
{
    Serial.begin(9600);
    while (!Serial) { delay(100); }
    Serial.println("Serial Initialized");

    //pinMode(SERVO_PIN, output);
    antenna_servo.attach(SERVO_PIN);

    my_rng.state = 0xDEADBEEF; /*(uint64_t)time(0) ^ (uintptr_t)&rng ^ (uint64_t)clock();*/;
    my_rng.inc = 0xCAFEBABE - 1; /*((uint64_t)time(0) << 32 | (uint64_t)rand()) | 1; /* Make sure inc is odd */

    altitude = 1000;

    /* inputs */
    init_lat = 28.058673159401824;
    init_long = -82.41544355009162;

    new_lat = 28.060129882339137;
    new_long = -82.41667208201783;

    delta_long = init_long - new_long;
    delta_lat = init_lat - new_lat;

    /* output */
    /* Serial.printf("phi: %.3f\n", toDegrees(phi)); */
}

void loop() {
    randomWalk(&my_rng, &delta_lat, &delta_long);
    phi = posToPhi(init_lat, delta_lat, delta_long, altitude);
    elevation_angle = 90 - toDegrees(phi);
    antenna_servo.write(elevation_angle);
    delay(15);
    Serial.printf("PHI: %.3f, (%.4f, %.4f)\n", elevation_angle, delta_lat, delta_long);
}
