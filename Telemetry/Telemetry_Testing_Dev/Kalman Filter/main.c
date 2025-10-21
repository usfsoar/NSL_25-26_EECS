#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "kalman.h"
#include "matrix.h"
#include "pcg.h"

void updateState(pcg32_random_t *rng, matrix * true_state, double dt, double sigma) {
    double pos, vel, acc;
    acc = sigma * pcg32_normal_r(rng);
    
    getElement(true_state, 1, 1, &pos);
    getElement(true_state, 2, 1, &vel);

    setElement(true_state, 1, 1, pos + vel*dt + acc*0.5*dt*dt);
    setElement(true_state, 2, 1, vel + acc*dt);
}

void readState(pcg32_random_t *rng, matrix * reading, matrix * true_state, double sigma) {
    double noise, noisy_pos;
    noise = sigma * pcg32_normal_r(rng);
    getElement(true_state, 1, 1, &noisy_pos);
    noisy_pos += noise;

    setElement(reading, 1, 1, noisy_pos);
}

int main(void) {
    pcg32_random_t rng;
    double t;
    double dt = 0.1;
    kalmanFilter *filter = NULL;
    const int states = 2;
    const int observations = 1;
    double sigma_a = 10;
    double sigma_z = 1;
    matrix *true_state = matrixCreate(states, 1);

    rng.state = (uint64_t)time(0) ^ (uintptr_t)&rng ^ (uint64_t)clock();
    rng.inc = ((uint64_t)time(0) << 32 | (uint64_t)rand()) | 1; /* Make sure inc is odd */
    filter = kalmanFilterCreate(states, observations);

    /* set filter matrices and vectors matrices
     * matrices are initially all 0s so no need to set them */
    setElement(filter->F_k, 1, 1, 1);
    setElement(filter->F_k, 1, 2, dt);
    setElement(filter->F_k, 2, 2, 1);

    setElement(filter->Q_k, 1, 1, pow(sigma_a, 2) * 0.25 * pow(dt,4));
    setElement(filter->Q_k, 1, 2, pow(sigma_a, 2) * 0.5 * pow(dt,3));
    setElement(filter->Q_k, 2, 1, pow(sigma_a, 2) * 0.5 * pow(dt,3));
    setElement(filter->Q_k, 2, 2, pow(sigma_a, 2) * pow(dt,2));

    setElement(filter->H_k, 1, 1, 1);
    
    setElement(filter->R_k, 1, 1, pow(sigma_z, 2));


    printf("time,true_pos,true_vel,est_pos,est_vel,measured_pos\n");
    for (t = 0; t < 50.0 + dt; t += dt) {
        printf("%.2f,%.4f,%.4f,%.4f,%.4f,%.4f\n", t,
                ELEM(true_state, 1, 1),
                ELEM(true_state, 2, 1),
                ELEM(filter->x_k, 1, 1),
                ELEM(filter->x_k, 2, 1),
                ELEM(filter->z_k, 1, 1));

        updateState(&rng, true_state, dt, sigma_a);
        readState(&rng, filter->z_k, true_state, sigma_z);

        kalmanFilterPredict(filter);
        kalmanFilterUpdate(filter);
    }
    
    matrixDestroy(true_state);
    kalmanFilterDestroy(filter);

    return 0;
}
