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
    if (filter == NULL) { return NULL; }

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
        !filter->S_k) {
        return NULL;
    }

    return filter;
}

int kalmanFilterDestroy(kalmanFilter *filter) {
    if (filter == NULL) { return 0; }
    if (filter->x_k)
        if (matrixDestroy(filter->x_k)) { return -1; }
    if (filter->x_k_prev)
        if (matrixDestroy(filter->x_k_prev)) { return -2; }
    if (filter->P_k)
        if (matrixDestroy(filter->P_k)) { return -3; }
    if (filter->P_k_prev)
        if (matrixDestroy(filter->P_k_prev)) { return -4; }
    if (filter->F_k)
        if (matrixDestroy(filter->F_k)) { return -5; }
    if (filter->H_k)
        if (matrixDestroy(filter->H_k)) { return -6; }
    if (filter->z_k)
        if (matrixDestroy(filter->z_k)) { return -7; }
    if (filter->Q_k)
        if (matrixDestroy(filter->Q_k)) { return -8; }
    if (filter->R_k)
        if (matrixDestroy(filter->R_k)) { return -9; }
    if (filter->K_k)
        if (matrixDestroy(filter->K_k)) { return -10; }
    if (filter->y_k)
        if (matrixDestroy(filter->y_k)) { return -11; }
    if (filter->S_k)
        if (matrixDestroy(filter->S_k)) { return -12; }

    return 0;
}

void kalmanFilterPredict(kalmanFilter *filter) {
    matrix *F_kt = matrixCreate(filter->F_k->cols, filter->F_k->rows); /* size for transpose */
    matrix *temp = matrixCreate(filter->F_k->rows, filter->P_k->cols);

    /* state estimate_vec */
    product(filter->F_k, filter->x_k_prev, filter->x_k);

    /* covar estimate_vec */
    transpose(filter->F_k, F_kt);
    product(filter->P_k_prev, F_kt, filter->P_k);
    product(filter->F_k, filter->P_k, temp);
    sum(temp, filter->Q_k, filter->P_k);
    
    /* cleanup */
    matrixDestroy(F_kt);
    matrixDestroy(temp);
}

void kalmanFilterUpdate(kalmanFilter *filter) {
    double mahalanobis_distance = 0;
    const double OUTLIER = 1e4; /* works well for test data */
    int i;
    matrix *vec = matrixCreate(filter->H_k->rows, 1);
    matrix *temp1 = matrixCreate(filter->P_k->rows, filter->H_k->rows);
    matrix *temp2 = matrixCreate(filter->H_k->rows, filter->H_k->rows);
    matrix *H_kt = matrixCreate(filter->H_k->cols, filter->H_k->rows); /* size for transpose */
    matrix *K_kt = matrixCreate(filter->K_k->cols, filter->K_k->rows); /* size for transpose */
    matrix *I = matrixCreate(filter->K_k->rows, filter->H_k->cols);
    matrix *pre_trans = matrixCreate(filter->K_k->rows, filter->H_k->cols);
    matrix *trans = matrixCreate(filter->H_k->cols, filter->K_k->rows); /* size for transpose */
    matrix *S_k_inv = matrixCreate(filter->S_k->rows, filter->S_k->cols);
    transpose(filter->H_k, H_kt);
    identity(I);

    /* update pre-fit residual / innovation */
    product(filter->H_k, filter->x_k, vec);
    difference(filter->z_k, vec, filter->y_k);

    /* check for outlier */
    for (i = 1; i <= filter->y_k->rows; i++) {
        if (ELEM(filter->S_k, i, i) == 0) { continue; }
        mahalanobis_distance += sqrt( pow(ELEM(filter->y_k, i, 1), 2) / ELEM(filter->S_k, i, i) );
    }
    /* reject outlier */
    if (mahalanobis_distance >= OUTLIER) { return; } 

    /* update innovation covariance */
    product(filter->P_k, H_kt, temp1);
    product(filter->H_k, temp1, temp2);
    sum(temp2, filter->R_k, filter->S_k);

    /* resize temp1 matrix */
    matrixResize(temp1, filter->H_k->cols, filter->H_k->rows);

    /* update kalman gain */
    inverse(filter->S_k, S_k_inv);
    product(H_kt, S_k_inv, temp1);
    product(filter->P_k, temp1, filter->K_k);

    /* resize vec */
    matrixResize(vec, filter->x_k->rows, 1);

    /* update state estimate_vec */
    filter->x_k_prev = matrixCopy(filter->x_k);
    product(filter->K_k, filter->y_k, vec);
    sum(filter->x_k_prev, vec, filter->x_k);

    /* resize temp matrices */
    matrixResize(temp1, filter->P_k->rows, filter->P_k->cols);
    matrixResize(temp2, filter->H_k->rows, filter->H_k->cols);

    /* update estimate_vec covar */
    product(filter->K_k, filter->H_k, trans); /* borrowing trans */
    difference(I, trans, pre_trans);
    transpose(pre_trans, trans);
    product(filter->P_k, trans, filter->P_k_prev); /* borrowing P_k_prev */
    product(pre_trans, filter->P_k_prev, temp1);
    transpose(filter->K_k, K_kt);
    product(filter->R_k, K_kt, temp2);
    product(filter->K_k, temp2, trans); /* borrowing trans */
    sum(temp1, trans, filter->P_k);

    /* set past values equal to current values */
    filter->x_k_prev = matrixCopy(filter->x_k);
    filter->P_k_prev = matrixCopy(filter->P_k);

    /* cleanup */
    matrixDestroy(vec);
    matrixDestroy(temp1);
    matrixDestroy(temp2);
    matrixDestroy(H_kt);
    matrixDestroy(K_kt);
    matrixDestroy(pre_trans);
    matrixDestroy(trans);
    matrixDestroy(S_k_inv);
    matrixDestroy(I);
}

#endif
