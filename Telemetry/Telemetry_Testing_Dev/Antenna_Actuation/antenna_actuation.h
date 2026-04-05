#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535
#endif
/* earth is approximately flat */

double toDegrees(double radians) { return radians * 180.0 / M_PI; }
double toRadians(double degrees) { return degrees * M_PI / 180.0; }

double posToPhi(double init_lat, double delta_lat, double delta_long, double altitude) {
    /* altitude must be in meters */
    double EARTH_RADIUS = 6.371e6;
    double x, y, z;
    double rho;

    double delta_lat_rads = toRadians(delta_lat);
    double delta_long_rads = toRadians(delta_long);
    double init_lat_rads = toRadians(init_lat);

    x = EARTH_RADIUS * delta_long_rads * cos(init_lat_rads + delta_lat_rads);
    y = EARTH_RADIUS * delta_lat_rads;
    z = altitude;
    
    rho = sqrt(x*x + y*y + z*z);
    return acos(z / rho);
}
