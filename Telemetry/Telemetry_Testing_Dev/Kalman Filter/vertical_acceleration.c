#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "matrix.h"

int vectorComponent(matrix *vec, matrix *quat, matrix *unit_dir_vec, double *magnitude) {
    matrix *quat_prime, *unit_dir_quat, *quat_temp, *unit_dir_vec_prime;
    double num = 0;
    *magnitude = 0;

    if (vec->cols != 1 || vec->rows != 3) { return -1; }
    if (quat->cols != 1 || quat->rows != 4) { return -2; }
    if (unit_dir_vec->cols != 1 || unit_dir_vec->rows != 3) { return -3; }

    unit_dir_quat = matrixCreate(4, 1);
    quat_temp = matrixCreate(4, 1);
    unit_dir_vec_prime = matrixCreate(3, 1);

    quat_prime = matrixCopy(quat);
    getElement(quat, 2, 1, &num);
    setElement(quat_prime, 2, 1, -num);
    getElement(quat, 3, 1, &num);
    setElement(quat_prime, 3, 1, -num);
    getElement(quat, 4, 1, &num);
    setElement(quat_prime, 4, 1, -num);

    /* unit_dir_vec -> unit_dir_quat */
    getElement(unit_dir_vec, 1, 1, &num);
    setElement(unit_dir_quat, 2, 1, num);
    getElement(unit_dir_vec, 2, 1, &num);
    setElement(unit_dir_quat, 3, 1, num);
    getElement(unit_dir_vec, 3, 1, &num);
    setElement(unit_dir_quat, 4, 1, num);

    /* tranform unit_dir quat */
    quaternionProduct(quat_prime, unit_dir_quat, quat_temp);
    quaternionProduct(quat_temp, quat, unit_dir_quat);

    /* unit_dir_quat -> unit_dir_vec_prime */
    getElement(unit_dir_quat, 2, 1, &num);
    setElement(unit_dir_vec_prime, 1, 1, num);
    getElement(unit_dir_quat, 3, 1, &num);
    setElement(unit_dir_vec_prime, 2, 1, num);
    getElement(unit_dir_quat, 4, 1, &num);
    setElement(unit_dir_vec_prime, 3, 1, num);

    /* a*b = |a|*|b|*cos(θ) */
    dotProduct(vec, unit_dir_vec_prime, magnitude);

    matrixDestroy(quat_prime);
    matrixDestroy(quat_temp);
    matrixDestroy(unit_dir_quat);
    matrixDestroy(unit_dir_vec_prime);

    return 0;
}

int main(void)
{
    enum { SIZE = 1024 };
    FILE *fp = NULL;
    char row[SIZE];
    char *token;
    int input = 0;
    double num = 0;

    matrix *quat, *acc_vec, *dir_vec;

    quat = matrixCreate(4, 1);
    acc_vec = matrixCreate(3, 1);
    dir_vec = matrixCreate(3, 1);

    setElement(dir_vec, 1, 1, 1);

    fp = fopen("imu.csv", "r");
    if (fp == NULL) {
        printf("File could not be opened\n");
        return -1;
    }

    fgets(row, SIZE, fp); /* skip first line */
    while (feof(fp) != 1) {
        input = 0;
        fgets(row, SIZE, fp);

        token = strtok(row, ",");
        /* skip first 2 cols */
        token = strtok(NULL, ",");
        token = strtok(NULL, ",");

        while (token != NULL) {
            num = atof(token);

            switch (input) {
                case 0:
                    setElement(acc_vec, 1, 1, num);
                    break;
                case 1:
                    setElement(acc_vec, 2, 1, num);
                    break;
                case 2:
                    setElement(acc_vec, 3, 1, num);
                    break;
                case 3:
                    setElement(quat, 1, 1, num);
                    break;
                case 4:
                    setElement(quat, 2, 1, num);
                    break;
                case 5:
                    setElement(quat, 3, 1, num);
                    break;
                case 6:
                    setElement(quat, 4, 1, num);
                    break;
                default:
                    printf("Error: input value\n");
                    break;
            }
            input++;
            token = strtok(NULL, ",");
        }

        vectorComponent(acc_vec, quat, dir_vec, &num);
        printf("a = % .2f\n", num);

    }

    matrixDestroy(quat);
    matrixDestroy(acc_vec);
    matrixDestroy(dir_vec);
    fclose(fp);

    return 0;
}

    
