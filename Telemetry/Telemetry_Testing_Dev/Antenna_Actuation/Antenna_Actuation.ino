#include <stdio.h>
#include "antenna_actuation.h"

int setup(void)
{
    double init_lat, init_long;
    double new_long, new_lat;
    double delta_long, delta_lat;
    double phi;
    double altitude = 10000; /* meters */

    /* inputs */
    init_lat = 28.058673159401824;
    init_long = -82.41544355009162;

    new_lat = 28.060129882339137;
    new_long = -82.41667208201783;

    delta_long = init_long - new_long;
    delta_lat = init_lat - new_lat;

    /* output */
    phi = posToPhi(init_lat, delta_lat, delta_long, altitude);
    Serial.printf("phi: %.3f\n", toDegrees(phi));
}

int loop(void)
{
}
