#ifndef KALMAN_H
#define KALMAN_H

#include <stdio.h>
#include "matrix.h"

typedef struct {
    /* chose not to implement control matrices */
    matrix *x_k;      /* state prediction vector               @ time k   */
    matrix *x_k_prev; /* state prediction vector               @ time k-1 */
    matrix *P_k;      /* estimate_vec covar prediction         @ time k   */
    matrix *P_k_prev; /* estimate_vec covar prediction         @ time k-1 */
    matrix *F_k;      /* linear transformation of dynamics     @ time k   */
    matrix *H_k;      /* linear transform of observation model @ time k   */
    matrix *z_k;      /* observation vector                    @ time k   */
    matrix *Q_k;      /* dynamics model noise covar            @ time k   */
    matrix *R_k;      /* observation noise covar               @ time k   */
    matrix *K_k;      /* optimal kalman gain matrix            @ time k   */
    matrix *y_k;      /* innovation (measurement residual)     @ time k   */
    matrix *S_k;      /* innovation covar                      @ time k   */

    /* helper matrices 
     * N = state_size 
     * M = observation_size */
    matrix *F_kt;    /* F_k transpose */
    matrix *H_kt;    /* H_k transpose */
    matrix *K_kt;    /* K_k transpose */
    matrix *I;       /* Identity      */
    matrix *vec_N;
    matrix *vec_M;
    matrix *temp_N_N_1;
    matrix *temp_N_N_2;
    matrix *temp_N_M_1;
    matrix *temp_M_N_1;
    matrix *temp_M_M_1;
    matrix *pre_trans;
    matrix *trans;
} kalmanFilter;

/* prototypes */
kalmanFilter * kalmanFilterCreate(int state_size, int observation_size);
int kalmanFilterDestroy(kalmanFilter *filter);
void kalmanFilterPredict(kalmanFilter *filter);
void kalmanFilterUpdate(kalmanFilter *filter);


kalmanFilter * kalmanFilterCreate(int state_size, int observation_size) {
    /* allocate memory for the filter itself */
    kalmanFilter *filter; 
    if (observation_size > state_size) { return NULL; }

    filter = (kalmanFilter *)malloc(sizeof(kalmanFilter));
    if (!filter) { return NULL; }

    filter->x_k = NULL;
    filter->x_k_prev = NULL;
    filter->P_k = NULL;
    filter->P_k_prev = NULL;
    filter->F_k = NULL;
    filter->H_k = NULL;
    filter->z_k = NULL;
    filter->Q_k = NULL;
    filter->R_k = NULL;
    filter->K_k = NULL;
    filter->y_k = NULL;
    filter->S_k = NULL;

    /* helper matrices */
    filter->F_kt = NULL;
    filter->H_kt = NULL;
    filter->K_kt = NULL;
    filter->I = NULL;
    filter->vec_N = NULL;
    filter->vec_M = NULL;
    filter->temp_N_N_1 = NULL;
    filter->temp_N_N_2 = NULL;
    filter->temp_N_M_1 = NULL;
    filter->temp_M_N_1 = NULL;
    filter->temp_M_M_1 = NULL;
    filter->pre_trans = NULL;
    filter->trans = NULL;

    filter->x_k = matrixCreate(state_size, 1);
    filter->x_k_prev = matrixCreate(state_size, 1);
    filter->P_k = matrixCreate(state_size, state_size);
    filter->P_k_prev = matrixCreate(state_size, state_size);
    filter->F_k = matrixCreate(state_size, state_size);
    filter->H_k = matrixCreate(observation_size, state_size);
    filter->z_k = matrixCreate(observation_size, 1);
    filter->Q_k = matrixCreate(state_size, state_size);
    filter->R_k = matrixCreate(observation_size, observation_size);
    filter->K_k = matrixCreate(state_size, observation_size);
    filter->y_k = matrixCreate(observation_size, 1);
    filter->S_k = matrixCreate(observation_size, observation_size);

    filter->F_kt = matrixCreate(state_size, state_size);
    filter->H_kt = matrixCreate(state_size, observation_size);
    filter->K_kt = matrixCreate(observation_size, state_size);
    filter->I = matrixCreate(state_size, state_size);
    filter->vec_N = matrixCreate(state_size, 1);
    filter->vec_M = matrixCreate(observation_size, 1);
    filter->temp_N_N_1 = matrixCreate(state_size, state_size);
    filter->temp_N_N_2 = matrixCreate(state_size, state_size);
    filter->temp_N_M_1 = matrixCreate(state_size, observation_size);
    filter->temp_M_N_1 = matrixCreate(observation_size, state_size);
    filter->temp_M_M_1 = matrixCreate(observation_size, observation_size);
    filter->pre_trans = matrixCreate(state_size, state_size);
    filter->trans = matrixCreate(state_size, state_size);

    if (!filter->x_k ||
        !filter->x_k_prev ||
        !filter->P_k ||
        !filter->P_k_prev ||
        !filter->F_k ||
        !filter->H_k ||
        !filter->z_k ||
        !filter->Q_k ||
        !filter->R_k ||
        !filter->K_k ||
        !filter->y_k ||
        !filter->S_k ||
        !filter->F_kt ||
        !filter->H_kt ||
        !filter->K_kt ||
        !filter->I ||
        !filter->vec_N ||
        !filter->vec_M ||
        !filter->temp_N_N_1 ||
        !filter->temp_N_N_2 ||
        !filter->temp_N_M_1 ||
        !filter->temp_M_N_1 ||
        !filter->temp_M_M_1 ||
        !filter->pre_trans ||
        !filter->trans) {
        kalmanFilterDestroy(filter);
        return NULL;
    }

    identity(filter->I);
    return filter;
}

int kalmanFilterDestroy(kalmanFilter *filter) {
    int failed = 0;
    if (filter == NULL) { return 0; }
    if (filter->x_k)
        if (matrixDestroy(filter->x_k)) { failed = -1; }
    if (filter->x_k_prev)
        if (matrixDestroy(filter->x_k_prev)) { failed = -2; }
    if (filter->P_k)
        if (matrixDestroy(filter->P_k)) { failed = -3; }
    if (filter->P_k_prev)
        if (matrixDestroy(filter->P_k_prev)) { failed = -4; }
    if (filter->F_k)
        if (matrixDestroy(filter->F_k)) { failed = -5; }
    if (filter->H_k)
        if (matrixDestroy(filter->H_k)) { failed = -6; }
    if (filter->z_k)
        if (matrixDestroy(filter->z_k)) { failed = -7; }
    if (filter->Q_k)
        if (matrixDestroy(filter->Q_k)) { failed = -8; }
    if (filter->R_k)
        if (matrixDestroy(filter->R_k)) { failed = -9; }
    if (filter->K_k)
        if (matrixDestroy(filter->K_k)) { failed = -10; }
    if (filter->y_k)
        if (matrixDestroy(filter->y_k)) { failed = -11; }
    if (filter->S_k)
        if (matrixDestroy(filter->S_k)) { failed = -12; }
    if (filter->F_kt)
        if (matrixDestroy(filter->F_kt)) { failed = -13; }
    if (filter->H_kt)
        if (matrixDestroy(filter->H_kt)) { failed = -14; }
    if (filter->K_kt)
        if (matrixDestroy(filter->K_kt)) { failed = -15; }
    if (filter->I)
        if (matrixDestroy(filter->I)) { failed = -16; }
    if (filter->vec_N)
        if (matrixDestroy(filter->vec_N)) { failed = -17; }
    if (filter->vec_M)
        if (matrixDestroy(filter->vec_M)) { failed = -18; }
    if (filter->temp_N_N_1)
        if (matrixDestroy(filter->temp_N_N_1)) { failed = -19; }
    if (filter->temp_N_N_2)
        if (matrixDestroy(filter->temp_N_N_2)) { failed = -20; }
    if (filter->temp_N_M_1)
        if (matrixDestroy(filter->temp_N_M_1)) { failed = -21; }
    if (filter->temp_M_N_1)
        if (matrixDestroy(filter->temp_M_N_1)) { failed = -22; }
    if (filter->temp_M_M_1)
        if (matrixDestroy(filter->temp_M_M_1)) { failed = -23; }
    if (filter->pre_trans)
        if (matrixDestroy(filter->pre_trans)) { failed = -24; }
    if (filter->trans)
        if (matrixDestroy(filter->trans)) { failed = -25; }

    free(filter);
    /* show one of the failed attempts if any */
    return failed; 
}

void kalmanFilterPredict(kalmanFilter *filter) {
    /* state estimate_vec */
    product(filter->F_k, filter->x_k_prev, filter->x_k);

    /* covar estimate_vec */
    transpose(filter->F_k, filter->F_kt);
    product(filter->P_k_prev, filter->F_kt, filter->P_k);
    product(filter->F_k, filter->P_k, filter->temp_N_N_1);
    sum(filter->temp_N_N_1, filter->Q_k, filter->P_k);
}

void kalmanFilterUpdate(kalmanFilter *filter) {
    double mahalanobis_distance = 0;
    const double OUTLIER = 1e4; /* works well for test data */
    int i;
    transpose(filter->H_k, filter->H_kt);

    /* update pre-fit residual / innovation */
    product(filter->H_k, filter->x_k, filter->vec_M);
    difference(filter->z_k, filter->vec_M, filter->y_k);

    /* check for outlier */
    for (i = 1; i <= filter->y_k->rows; i++) {
        if (ELEM(filter->S_k, i, i) == 0) { continue; }
        mahalanobis_distance += sqrt( pow(ELEM(filter->y_k, i, 1), 2) / ELEM(filter->S_k, i, i) );
    }
    /* reject outlier */
    if (mahalanobis_distance >= OUTLIER) { 
        /* set past values equal to current values */
        filter->x_k_prev = matrixCopy(filter->x_k);
        filter->P_k_prev = matrixCopy(filter->P_k);
        return;
    } 

    /* update innovation covariance */
    product(filter->P_k, filter->H_kt, filter->temp_N_M_1);
    product(filter->H_k, filter->temp_N_M_1, filter->temp_M_M_1);
    sum(filter->temp_M_M_1, filter->R_k, filter->S_k);

    /* update kalman gain */
    inverse(filter->S_k, filter->temp_M_M_1);
    product(filter->H_kt, filter->temp_M_M_1, filter->temp_N_M_1);
    product(filter->P_k, filter->temp_N_M_1, filter->K_k);

    /* update state estimate_vec */
    filter->x_k_prev = matrixCopy(filter->x_k);
    product(filter->K_k, filter->y_k, filter->vec_N);
    sum(filter->x_k_prev, filter->vec_N, filter->x_k);

    /* update estimate_vec covar */
    product(filter->K_k, filter->H_k, filter->temp_N_N_1);
    difference(filter->I, filter->temp_N_N_1, filter->pre_trans);
    transpose(filter->pre_trans, filter->trans);
    product(filter->P_k, filter->trans, filter->temp_N_N_2);
    product(filter->pre_trans, filter->temp_N_N_2, filter->temp_N_N_1);
    transpose(filter->K_k, filter->K_kt);
    product(filter->R_k, filter->K_kt, filter->temp_M_N_1);
    product(filter->K_k, filter->temp_M_N_1, filter->temp_N_N_2);
    sum(filter->temp_N_N_1, filter->temp_N_N_2, filter->P_k);

    /* set past values equal to current values */
    matrixCopyData(filter->x_k, filter->x_k_prev);
    matrixCopyData(filter->P_k, filter->P_k_prev);
}

#endif
