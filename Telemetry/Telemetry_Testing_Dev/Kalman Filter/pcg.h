#ifndef PCG_H
#define PCG_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* pi to 20 decimal places */
const double PI = 3.14159265358979323846;

/* Define the PCG random number generator struct and function */
typedef struct { uint64_t state; uint64_t inc; } pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t* rng) {
    uint32_t xorshifted, rot;
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005UL + (rng->inc | 1);
    xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_bounded_r(pcg32_random_t* rng, uint32_t bound) { /* [0, bound) */
    uint32_t threshold = -bound % bound;
    while(1) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}

double pcg32_unit_r(pcg32_random_t* rng) { /* [0, 1) */
    return ldexp(pcg32_random_r(rng), -32);
}

double pcg32_normal_r(pcg32_random_t* rng) { /* standard normal distribution */
    double x1, x2;
    x1 = ldexp(pcg32_random_r(rng), -32);
    x2 = ldexp(pcg32_random_r(rng), -32);

    return sqrt(-2.0*log(x1)) * cos(2.0*PI*x2);
}

#endif
