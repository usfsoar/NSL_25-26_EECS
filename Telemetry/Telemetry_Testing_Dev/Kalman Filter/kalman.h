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
    filter->z_k = matrixCreate(state_size, 1);
    filter->Q_k = matrixCreate(state_size, state_size);
    filter->R_k = matrixCreate(state_size, state_size);
    filter->K_k = matrixCreate(state_size, state_size);
    filter->y_k = matrixCreate(state_size, 1);
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
    matrix *mtx_temp2 = matrixCreate(filter->F_k->rows, filter->F_k->cols);

    /* state estimate_vec */
    product(filter->F_k, filter->x_k_prev, filter->x_k);

    /* covar estimate_vec */
    transpose(filter->F_k, F_kt);
    product(filter->P_k_prev, F_kt, filter->P_k);
    product(filter->F_k, filter->P_k, mtx_temp2);
    sum(mtx_temp2, filter->Q_k, filter->P_k);
    
    /* cleanup */
    matrixDestroy(F_kt);
    matrixDestroy(mtx_temp2);
}

void kalmanFilterUpdate(kalmanFilter *filter) {
    matrix *vec = matrixCreate(filter->H_k->rows, filter->x_k->cols);
    matrix *mtx_temp1 = matrixCreate(filter->P_k->rows, filter->H_k->rows);
    matrix *mtx_temp2 = matrixCreate(filter->H_k->rows, filter->H_k->rows);
    matrix *mtx_temp3 = matrixCreate(filter->H_k->rows, filter->H_k->cols);
    matrix *H_kt = matrixCreate(filter->H_k->cols, filter->H_k->rows); /* size for transpose */
    matrix *I = matrixCreate(filter->K_k->rows, filter->H_k->cols);
    matrix *pre_trans = matrixCreate(filter->K_k->rows, filter->H_k->cols);
    matrix *trans = matrixCreate(filter->H_k->cols, filter->K_k->rows); /* size for transpose */
    matrix *S_k_inv = matrixCreate(filter->S_k->rows, filter->S_k->cols);
    transpose(filter->H_k, H_kt);
    identity(I);

    printf("H_k[x_k]\n");
    /* update pre-fit residual / innovation */
    product(filter->H_k, filter->x_k, vec);
    difference(filter->z_k, vec, filter->y_k);

    /* update innovation covariance */
    printf("P_k[H_kt]\n");
    product(filter->P_k, H_kt, mtx_temp1);
    printf("H_k[prev]\n");
    product(filter->H_k, mtx_temp1, mtx_temp2);
    sum(mtx_temp2, filter->R_k, filter->S_k);

    /* update kalman gain */
    inverse(filter->S_k, S_k_inv);
    printf("H_kt[S_k_inv]\n");
    product(H_kt, S_k_inv, mtx_temp3);
    printf("P_k[prev]\n");
    product(filter->P_k, mtx_temp3, filter->K_k);

    /* update state estimate_vec */
    filter->x_k_prev = matrixCopy(filter->x_k);
    product(filter->K_k, filter->y_k, vec);
    sum(filter->x_k_prev, vec, filter->x_k);

    /* update estimate_vec covar */
    product(filter->K_k, filter->H_k, trans); /* borrowing trans */
    difference(I, trans, pre_trans);
    transpose(pre_trans, trans);
    product(filter->P_k, trans, filter->P_k_prev); /* borrowing P_k_prev */
    product(pre_trans, filter->P_k_prev, mtx_temp2);
    transpose(filter->K_k, trans);
    product(filter->R_k, trans, filter->P_k_prev); /* borrowing P_k_prev again */
    product(filter->K_k, filter->P_k_prev, trans); /* borrowing trans */
    sum(mtx_temp2, trans, filter->P_k);

    /* set past values equal to current values */
    filter->x_k_prev = matrixCopy(filter->x_k);
    filter->P_k_prev = matrixCopy(filter->P_k);

    /* cleanup */
    matrixDestroy(vec);
    matrixDestroy(mtx_temp1);
    matrixDestroy(mtx_temp2);
    matrixDestroy(mtx_temp3);
    matrixDestroy(H_kt);
    matrixDestroy(pre_trans);
    matrixDestroy(trans);
    matrixDestroy(S_k_inv);
    matrixDestroy(I);
}

#endif
