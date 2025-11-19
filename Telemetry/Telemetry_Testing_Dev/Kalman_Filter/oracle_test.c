#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "kalman.h"
#include "matrix.h"
#include "pcg.h"

void updateState(pcg32_random_t *rng, matrix * true_state, double dt, double sigma) {
    double pos, vel, acc, jerk;
    jerk = sigma * pcg32_normal_r(rng);
    
    getElement(true_state, 1, 1, &pos);
    getElement(true_state, 2, 1, &vel);
    getElement(true_state, 3, 1, &acc);

    setElement(true_state, 1, 1, pos + vel*dt + acc*0.5*dt*dt + jerk*(1.0/6)*dt*dt*dt);
    setElement(true_state, 2, 1, vel + acc*dt + jerk*0.5*dt*dt);
    setElement(true_state, 3, 1, acc + jerk*dt);
}

void readState(pcg32_random_t *rng, matrix * reading, matrix * true_state, double sigma_s, double sigma_a) {
    double noise_s, noise_a, noisy_pos, noisy_acc;
    noise_s = sigma_s * pcg32_normal_r(rng);
    noise_a = sigma_a * pcg32_normal_r(rng);

    getElement(true_state, 1, 1, &noisy_pos);
    getElement(true_state, 3, 1, &noisy_acc);
    noisy_pos += noise_s;
    noisy_acc += noise_a;

    setElement(reading, 1, 1, noisy_pos);
    setElement(reading, 2, 1, noisy_acc);
}

int main(void) {
    pcg32_random_t rng;
    double t;
    const double dt = 0.05;
    const int states = 3;
    const int observations = 2;

    /* works badly when sigma_j > sigma a
     * the higher sigma_j, the worse our physical
     * model will give a decent prediction */
    const double sigma_j = 0.2;
    const double sigma_s = 0.255;
    const double sigma_a = 0.179;
    matrix *true_state = matrixCreate(states, 1);
    kalmanFilter *filter = NULL;

    rng.state = 0xDEADBEEF; /*(uint64_t)time(0) ^ (uintptr_t)&rng ^ (uint64_t)clock();*/
    rng.inc = 0xCAFEBABE - 1; /*((uint64_t)time(0) << 32 | (uint64_t)rand()) | 1; /* Make sure inc is odd */
    filter = kalmanFilterCreate(states, observations);

    /* set filter matrices and vectors matrices
     * matrices are initially all 0s so no need to set them */
    setElement(filter->F_k, 1, 1, 1);
    setElement(filter->F_k, 1, 2, dt);
    setElement(filter->F_k, 1, 3, 0.5*dt*dt);
    setElement(filter->F_k, 2, 2, 1);
    setElement(filter->F_k, 2, 3, dt);
    setElement(filter->F_k, 3, 3, 1);

    setElement(filter->H_k, 1, 1, 1);
    setElement(filter->H_k, 2, 3, 1);

    /* process covar */
    setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
    setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
    setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
    setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
    setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
    setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
    setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
    setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
    setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));
    
    /* reading covar */
    setElement(filter->R_k, 1, 1, sigma_s * sigma_s);
    setElement(filter->R_k, 2, 2, sigma_a * sigma_a);

    /* uncertainty of initial velocity and acceleration */
    setElement(filter->P_k_prev, 2, 2, 100);
    setElement(filter->P_k_prev, 3, 3, 100);

    printf("time,true_pos,true_vel,true_acc,est_pos,est_vel,measured_pos\n");
    for (t = 0; t < 50.0 + dt; t += dt) {
        printf("%.2f,%.4f,%.4f,%.4f,%.4f,%.4f\n", t,
                ELEM(true_state, 1, 1),
                ELEM(true_state, 2, 1),
                ELEM(filter->x_k, 1, 1),
                ELEM(filter->x_k, 2, 1),
                ELEM(filter->z_k, 1, 1));

        updateState(&rng, true_state, dt, sigma_j);
        readState(&rng, filter->z_k, true_state, sigma_s, sigma_a);

        kalmanFilterPredict(filter);
        kalmanFilterUpdate(filter);
    }
    
    matrixDestroy(true_state);
    kalmanFilterDestroy(filter);

    return 0;
}
