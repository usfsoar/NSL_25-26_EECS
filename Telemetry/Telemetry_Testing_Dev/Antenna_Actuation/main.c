#include <stdio.h>
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

int main(void)
{
    /* inputs */
    double init_lat, init_long;
    double new_long, new_lat;
    double delta_long, delta_lat;
    double phi;
    double altitude = 100;

    init_lat = 28.058673159401824;
    init_long = -82.41544355009162;

    new_lat = 28.060129882339137;
    new_long = -82.41667208201783;

    delta_long = init_long - new_long;
    delta_lat = init_lat - new_lat;

    /* output */
    phi = posToPhi(init_lat, delta_lat, delta_long, altitude);
    printf("phi: %.3f\n", toDegrees(phi));
    
}
