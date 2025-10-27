#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "matrix.h"



int main(void)
{
    enum { SIZE = 1024 };
    FILE *fp;
    char row[SIZE];
    char *token;
    int input = 0;
    double num;

    matrix *quat1, *quat2, *quat_temp, *acc_vec;

    quat1 = matrixCreate(4, 1);
    quat2 = matrixCreate(4, 1);
    quat_temp = matrixCreate(4, 1);
    acc_vec = matrixCreate(4, 1);

    fopen_s(&fp, "imu.csv", "r");

    fgets(row, SIZE, fp); /* skip first line */
    while (feof(fp) != 1) {
        input = 0;
        fgets(row, SIZE, fp);

        token = strtok(row, ",");
        /* skip first 2 cols */
        token = strtok(NULL, ",");
        token = strtok(NULL, ",");

        while (token != NULL) {
            printf("%s ", token);
            token = strtok(NULL, ",");
            num = atoi(token);

            switch (input) {
                case 0:
                    setElement(acc_vec, 2, 1, num);
                    break;
                case 1:
                    setElement(acc_vec, 3, 1, num);
                    break;
                case 2:
                    setElement(acc_vec, 4, 1, num);
                    break;
                case 3:
                    setElement(quat1, 1, 1, num);
                    setElement(quat2, 1, 1, num);
                    break;
                case 4:
                    setElement(quat1, 2, 1, num);
                    setElement(quat2, 2, 1, -num);
                    break;
                case 5:
                    setElement(quat1, 3, 1, num);
                    setElement(quat2, 3, 1, -num);
                    break;
                case 6:
                    setElement(quat1, 4, 1, num);
                    setElement(quat2, 4, 1, -num);
                    break;
                default:
                    printf("Error: input value\n");
                    break;
            }
            input++;
        }

        /* transform acc vector */
        quaternionProduct(quat2, acc_vec, quat_temp);
        quaternionProduct(quat_temp, quat1, acc_vec);

        printf("direction:\n");
        matrixPrint(acc_vec);
    }

    matrixDestroy(quat1);
    matrixDestroy(quat2);
    matrixDestroy(quat_temp);
    matrixDestroy(acc_vec);

    return 0;
}

    
