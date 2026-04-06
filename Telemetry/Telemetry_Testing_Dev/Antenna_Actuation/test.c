#include <stdio.h>
#include <assert.h>
#include "antenna_actuation.h"

double init_lat, init_long;
double new_long, new_lat;
double delta_long, delta_lat;
double phi, phi_deg;
double altitude; /* meters */
double elevation_angle;

int main(void)
{
    altitude = 1000;

    /* inputs */
    init_lat = 28.058673159401824;
    init_long = -82.41544355009162;

    new_lat = 28.060129882339137;
    new_long = -82.41667208201783;

    delta_long = init_long - new_long;
    delta_lat = init_lat - new_lat;

    /* output */
    phi = posToPhi(init_lat, delta_lat, delta_long, altitude);
    elevation_angle = 90 - toDegrees(phi);
    printf("elevation_angle: %.3f\n", elevation_angle);
    return 0;
}
