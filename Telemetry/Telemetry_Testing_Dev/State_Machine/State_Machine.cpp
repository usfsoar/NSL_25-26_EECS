#include <iostream>
#include <cmath>
using namespace std;

void get_sensor_input(float* altitude, float* velocity, float* acceleration) {
    cout << "Enter [altitude velocity acceleration]: ";
    cin >> *altitude >> *velocity >> *acceleration;
}

int main() {
    bool run = true;
    float altitude = 0;
    float velocity = 0;
    float acceleration = 0;
    const float max_altitude = 100;
    const float max_alt_tol = 10;
    int stage = 0;
    while (run) {
        get_sensor_input(&altitude, &velocity, &acceleration);
        float alt_diff = std::abs(altitude - max_altitude);
        switch(stage) {
            case 0:
                if((acceleration > 0) && (velocity > 0) && (altitude >= 0) && (alt_diff > max_alt_tol)) {
                    stage = 1;
                }
                break;
            case 1:
                if((acceleration <= 0) && (velocity > 0) && (altitude > 0) && (alt_diff > max_alt_tol)) {
                    stage = 2;
                }
                break;
            case 2:
                if((acceleration < 0) && (altitude > 0) && (alt_diff <= max_alt_tol)) {
                    stage = 3;
                }
                break;
            case 3:
                if((acceleration < 0) && (velocity < 0) && (altitude > 0) && (alt_diff > max_alt_tol)) {
                    stage = 4;
                }
                break;
            case 4:
                if((acceleration <= 0) && (velocity <= 0) && (altitude <= 0) && (alt_diff > max_alt_tol)) {
                    stage = 0;
                }
                break;
        }
        cout << "Current stage: " << stage << "\n";
    }
    return 0;
}