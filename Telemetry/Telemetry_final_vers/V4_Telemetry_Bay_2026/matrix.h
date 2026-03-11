#ifndef MATRIX_H
#define MATRIX_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> /* for pow */


typedef struct {
    int rows;
    int cols;
    double * data;
} matrix;


/* prototypes */
matrix * matrixCreate(int rows, int cols);
int matrixDestroy(matrix * mtx);
int matrixResize(matrix * mtx, int new_rows, int new_cols);
matrix * matrixCopy(matrix * mtx);
int matrixCopyData(matrix * in, matrix * out);
int setElement(matrix * mtx, int row, int col, double val);
int getElement(matrix * mtx, int row, int col, double * val);
int nRows(matrix * mtx, int * n);
int nCols(matrix * mtx, int * n);
int matrixPrint(matrix * mtx);
int transpose(matrix * in, matrix * out);
int sum(matrix * mtx1, matrix * mtx2, matrix * sum);
int difference(matrix * mtx1, matrix * mtx2, matrix * difference);
int product(matrix * mtx1, matrix * mtx2, matrix * prod);
int multiply(double coefficient, matrix * in, matrix * out);
int divide(double divisor, matrix * in, matrix * out);
int dotProduct(matrix * v1, matrix * v2, double * prod);
int vectorProjection(matrix * v1, matrix * v2, matrix * proj);
int quaternionProduct(matrix * q1, matrix * q2, matrix * prod);
int identity(matrix * mtx);
int isSquare(matrix * mtx);
int isDiagonal(matrix * mtx);
int isUpperTriangular(matrix * mtx);
int diagonal(matrix * v, matrix * mtx);
int adjoint(matrix * in, matrix * out);
int determinant(matrix * mtx, double * det);
int inverse(matrix * in, matrix * out);


/* Creates a "rows by cols" matrix with all values 0.    
 * Returns NULL if rows <= 0 or cols <= 0 and otherwise a
 * pointer to the new matrix.
 */
matrix * matrixCreate(int rows, int cols) {
    matrix * m;
    if (rows <= 0 || cols <= 0) { return NULL; }

    /* allocate a matrix structure */
    m = (matrix *) malloc(sizeof (matrix));

    if (m == NULL) {
        printf("Matrix Allocation Failed\n");
        exit(100);
    }

    /* set dimensions */
    m->rows = rows;
    m->cols = cols;

    /* allocate a double array of length rows * cols with 0s */
    m->data = (double *) calloc(rows * cols, sizeof (double));

    if (m->data == NULL) {
        printf("Matrix Data Allocation Failed");
        exit(101);
    }

    return m;
}

/* resizes matrix while purging data 
 * TODO: implement keeping previous data
 * i.e. ELEM(mtx, i, j) is the same so long as it still exists
 * would need to carefully move data around to keep matrix "intact" */
int matrixResize(matrix * mtx, int new_rows, int new_cols) {
    if (!mtx) { return -1; }
    if (new_rows <= 0 || new_cols <= 0) { return -2; }

    free(mtx->data);
    mtx->data = (double *) calloc(new_rows * new_cols, sizeof (double));

    if (mtx->data == NULL) {
        printf("Matrix Data Allocation Failed");
        exit(101);
    }

    mtx->rows = new_rows;
    mtx->cols = new_cols;

    return 0;
}

/* Deletes a matrix. Returns 0 if successful and -1 if mtx is NULL. */
int matrixDestroy(matrix * mtx) {
    if (!mtx) { return -1; }
    /* free mtx's data */
    assert (mtx->data);
    free(mtx->data);
    /* free mtx itself */
    free(mtx);

    return 0;
}

/* hope I dont shoot myself in the foot */
#define ELEM(mtx, row, col) \
    mtx->data[(col-1) * mtx->rows + (row-1)]

/* Copies a matrix. Returns NULL if mtx is NULL. */
matrix * matrixCopy(matrix * mtx) {
    matrix * cp;

    if (!mtx) { return NULL; }
    /* create a new matrix to hold the copy */
    cp = matrixCreate(mtx->rows, mtx->cols);

    /* copy mtx's data to cp's data */
    memcpy(cp->data, mtx->data, mtx->rows * mtx->cols * sizeof (double));

    return cp;
}

int matrixCopyData(matrix * in, matrix * out) {
    int row, col; 
    if (!in || !out) { return -1; }
    if (in->rows != out->rows ||
        in->cols != out->cols) {
        return -2;
    }

    for (row = 1; row <= in->rows; row++) {
        for (col = 1; col <= in->cols; col++) {
            ELEM(out, row, col) = ELEM(in, row, col);
        }
    }
    return 0;
}

/* Sets the (row, col) element of mtx to val. Returns 0 if
 * successful, -1 if mtx is NULL, and -2 if row or col are
 * outside of the dimensions of mtx.
 */
int setElement(matrix * mtx, int row, int col, double val) 
{
    if (!mtx) { return -1; }
    assert (mtx->data);
    if (row <= 0 || row > mtx->rows ||
        col <= 0 || col > mtx->cols) {
        return -2;
    }
    ELEM(mtx, row, col) = val;
    return 0;
}

/* Sets the reference val to the value of the (row, col) 
 * element of mtx.    Returns 0 if successful, -1 if either 
 * mtx or val is NULL, and -2 if row or col are outside of 
 * the dimensions of mtx.
 */
int getElement(matrix * mtx, int row, int col, double * val) {
    if (!mtx || !val) return -1;
    assert (mtx->data);
    if (row <= 0 || row > mtx->rows ||
        col <= 0 || col > mtx->cols) {
        return -2;
    }
    *val = ELEM(mtx, row, col);
    return 0;
}

/* Sets the reference n to the number of rows of mtx.
 * Returns 0 if successful and -1 if mtx or n is NULL.
 */
int nRows(matrix * mtx, int * n) {
    if (!mtx || !n) { return -1; }
    *n = mtx->rows;
    return 0;
}

/* Sets the reference n to the number of columns of mtx.
 * Returns 0 if successful and -1 if mtx is NULL.
 */
int nCols(matrix * mtx, int * n) {
    if (!mtx || !n) { return -1; }
    *n = mtx->rows;
    return 0;
}

/* Prints the matrix to stdout.    Returns 0 if successful 
 * and -1 if mtx is NULL.
 */
int matrixPrint(matrix * mtx) {
    int row, col;
    if (!mtx) { return -1; }
    
    for (row = 1; row <= mtx->rows; row++) {
        for (col = 1; col <= mtx->cols; col++) {
            /* Print the floating-point element with */
            /*    - either a - if negative or a space if positive */
            /*    - at least 3 spaces before the . */
            /*    - precision to the hundredths place */
            printf("% 6.2f ", ELEM(mtx, row, col));
        }
        /* separate rows by newlines */
        printf("\n");
    }
    return 0;
}

/* Writes the transpose of matrix in into matrix out.    
 * Returns 0 if successful, -1 if either in or out is NULL,
 * and -2 if the dimensions of in and out are incompatible.
 */
int transpose(matrix * in, matrix * out) {
    int row, col;

    if (!in || !out) { return -1; }
    if (in->rows != out->cols || in->cols != out->rows) {
        return -2;
    }
    for (row = 1; row <= in->rows; row++)
        for (col = 1; col <= in->cols; col++)
            ELEM(out, col, row) = ELEM(in, row, col);

    return 0;
}

/* Writes the sum of matrices mtx1 and mtx2 into matrix 
 * sum. Returns 0 if successful, -1 if any of the matrices 
 * are NULL, and -2 if the dimensions of the matrices are
 * incompatible.
 */
int sum(matrix * mtx1, matrix * mtx2, matrix * sum) {
    int row, col;

    if (!mtx1 || !mtx2 || !sum) { return -1; }
    if (mtx1->rows != mtx2->rows ||
        mtx1->rows != sum->rows ||
        mtx1->cols != mtx2->cols ||
        mtx1->cols != sum->cols) {
        return -2;
    }
    for (col = 1; col <= mtx1->cols; col++)
        for (row = 1; row <= mtx1->rows; row++)
            ELEM(sum, row, col) = ELEM(mtx1, row, col) + ELEM(mtx2, row, col);
    return 0;
}


int difference(matrix * mtx1, matrix * mtx2, matrix * difference) {
    int row, col;

    if (!mtx1 || !mtx2 || !difference) { return -1; }
    if (mtx1->rows != mtx2->rows ||
        mtx1->rows != difference->rows ||
        mtx1->cols != mtx2->cols ||
        mtx1->cols != difference->cols) {
        return -2;
    }
    for (col = 1; col <= mtx1->cols; col++)
        for (row = 1; row <= mtx1->rows; row++)
            ELEM(difference, row, col) = ELEM(mtx1, row, col) - ELEM(mtx2, row, col);
    return 0;
}

/* Writes the product of matrices mtx1 and mtx2 into matrix
 * prod.    Returns 0 if successful, -1 if any of the 
 * matrices are NULL, and -2 if the dimensions of the 
 * matrices are incompatible.
 */
int product(matrix * mtx1, matrix * mtx2, matrix * prod) {
    int row, col, k;

    if (!mtx1 || !mtx2 || !prod) {
        return -1;
    } else if (mtx1->cols != mtx2->rows) {
        printf("Error: input dimension mismatch (%d != %d)\n", mtx1->cols, mtx2->rows);
        return -2;
    } else if (mtx1->rows != prod->rows) {
        printf("Error: output row mismatch (%d != %d)\n", mtx1->rows, prod->rows);
        return -2;
    } else if (mtx2->cols != prod->cols) {
        printf("Error: output col mismatch (%d != %d)\n", mtx2->cols, prod->cols);
        return -2;
    }
    for (col = 1; col <= mtx2->cols; col++)
        for (row = 1; row <= mtx1->rows; row++) {
            double val = 0.0;
            for (k = 1; k <= mtx1->cols; k++)
                val += ELEM(mtx1, row, k) * ELEM(mtx2, k, col);
            ELEM(prod, row, col) = val;
        }
    return 0;
}

int multiply(double coefficient, matrix * in, matrix * out) {
    int row, col;

    if (!in || !out) { return -1; }
    if (in->rows != out->rows || in->cols != out->cols) { return -2; }

    for (row = 1; row <= in->rows; row++) {
        for (col = 1; col <= in->cols; col++) {
            ELEM(out, row, col) = coefficient * ELEM(in, row, col);
        }
    }
    return 0;
}

int divide(double divisor, matrix * in, matrix * out) {
    int row, col;

    if (!in || !out) { return -1; }
    if (in->rows != out->cols || in->cols != out->rows) { return -2; }

    for (row = 1; row <= in->rows; row++) {
        for (col = 1; col <= in->cols; col++) {
            ELEM(out, row, col) = ELEM(in, row, col) / divisor;
        }
    }
    return 0;
}

/* Writes the dot product of vectors v1 and v2 into 
 * reference prod. Returns 0 if successful, -1 if any of
 * v1, v2, or prod are NULL, -2 if either matrix is not a 
 * vector, and -3 if the vectors are of incompatible 
 * dimensions.
 */
int dotProduct(matrix * v1, matrix * v2, double * prod) {
    int i;
    *prod = 0;

    if (!v1 || !v2 || !prod) { return -1; }
    if (v1->cols != 1 || v2->cols != 1) { return -2; }
    if (v1->rows != v2->rows) { return -3; }

    for (i = 1; i <= v1->rows; i++)
        *prod += ELEM(v1, i, 1) * ELEM(v2, i, 1);
    return 0;
}

/* projection of v1 onto v2 */
int vectorProjection(matrix * v1, matrix * v2, matrix * proj) {
    int i;
    matrix *vec;
    double mag_v2 = 0;
    double scalar = 1;
    
    if (!v1 || !v2 || !proj) { return -1; }
    if (v1->cols != 1 || v2->cols != 1) { return -2; }
    if (v1->rows != v2->rows) { return -3; }
    if (proj->rows != v1->rows || proj->cols != 1) { return -4; }

    for (i = 1; i <= v1->rows; i++) { mag_v2 += pow(ELEM(v2, i, 1), 2); }

    vec = matrixCopy(v2);
    dotProduct(v1, v2, &scalar);
    scalar /= mag_v2;

    matrixDestroy(vec);
    return 0;
}

int quaternionProduct(matrix * q1, matrix * q2, matrix * prod) {
    double w1, w2, x1, x2, y1, y2, z1, z2;
    w1 = ELEM(q1, 1, 1);
    x1 = ELEM(q1, 2, 1);
    y1 = ELEM(q1, 3, 1);
    z1 = ELEM(q1, 4, 1);

    w2 = ELEM(q2, 1, 1);
    x2 = ELEM(q2, 2, 1);
    y2 = ELEM(q2, 3, 1);
    z2 = ELEM(q2, 4, 1);

    if (!q1 || !q2 || !prod) { return -1; }
    if (q1->cols != 1 || q1->rows != 4) { return -2; }
    if (q2->cols != 1 || q2->rows != 4) { return -3; }
    if (q2->cols != 1 || q2->rows != 4) { return -4; }

    ELEM(prod, 1, 1) = w1*w2 - x1*x2 - y1*y2 - z1*z2;
    ELEM(prod, 2, 1) = w1*x2 + x1*w2 + y1*z2 - z1*y2;
    ELEM(prod, 3, 1) = w1*y2 - x1*z2 + y1*w2 + z1*x2;
    ELEM(prod, 4, 1) = w2*z1 + x1*y2 - y1*x2 + z2*w1;

    return 0;
}

int identity(matrix * mtx) {
    int row, col;
    if (!mtx || mtx->rows != mtx->cols) { return -1; }
    for (col = 1; col <= mtx->cols; col++)
        for (row = 1; row <= mtx->rows; row++)
            if (row == col) {
                ELEM(mtx, row, col) = 1.0;
            } else {
                ELEM(mtx, row, col) = 0.0;
            }

    return 0;
}

int cofactor(matrix * in, matrix * out) {
    int row, col, i, j;
    int row_offset, col_offset;
    matrix * sub_mtx; /* minors */
    double cofact, det;
    
    /* temporarily borrow det for error handling */
    determinant(in, &det);
    if (!in || !out) { return -1; }
    if (in->rows != out->cols || in->cols != out->rows) { return -2; }
    if (!det) { return -3; }

    sub_mtx = matrixCreate(in->rows - 1, in->cols - 1);

    for (row = 1; row <= in->rows; row++) {
        for (col = 1; col <= in->cols; col++) {
            row_offset = 0;
            col_offset = 0;
            for (i = 1; i <= in->rows; i++) {
                for (j = 1; j <= in->cols; j++) {
                    if (j == 1) { col_offset = 0; }
                    if (j == col) { 
                        col_offset = -1;
                        continue;
                    }
                    if (i == row) { 
                        row_offset = -1;
                        col_offset = 0;
                        continue;
                    }

                    ELEM(sub_mtx, i + row_offset, j + col_offset) = ELEM(in, i, j);
                }
            }
            determinant(sub_mtx, &det);
            cofact = (row + col) % 2 == 0 ? det : -det; 

            ELEM(out, row, col) = cofact;
        }
    }

    matrixDestroy(sub_mtx);
    return 0;
}

int adjoint(matrix * in, matrix * out) {
    matrix * cofactor_mtx;

    if (!in || !out) { return -1; }
    if (in->rows != out->cols || in->cols != out->rows) {
        return -2;
    }
    cofactor_mtx = matrixCreate(in->rows, in->cols);
    if (cofactor(in, cofactor_mtx) == -3) {
        return -3;
    }
    transpose(cofactor_mtx, out);
    matrixDestroy(cofactor_mtx);

    return 0;
}

/* use diagonal product of row eschelon form 
 * 1. swapping 2 rows flips the sign of the determinant
 * 2. adding a constant multiple of one row to another does not affect determinant
 * 3. multiplying a row by a constant factor multiplies the determinant by that factor
 * returns -1 for non-square arrays or NULL matrices
 * returns -2 for gigantanorstis matrices (> MAX_SIZE)
 */
int determinant(matrix * mtx, double * det) {
    int row, col, i, j;
    matrix * mtx_copy;
    enum { MAX_SIZE = 100 };
    double diagonal[MAX_SIZE] = {0};
    double temp = 0;
    double scalar = 1;

    *det = 1;

    if (!mtx || mtx->rows != mtx->cols) { return -1; }
    if (mtx->rows >= MAX_SIZE) { return -2; }

    if (mtx->rows == 1) { 
        *det = ELEM(mtx, 1, 1);
        return 0;
    } else if (mtx->rows == 2) { 
        *det = ELEM(mtx, 1, 1) * ELEM(mtx, 2, 2) - ELEM(mtx, 1, 2) * ELEM(mtx, 2, 1);
        return 0;
    } else {
        mtx_copy = matrixCopy(mtx);
        /* traverse diagonal */
        for (col = 1; col <= mtx_copy->cols; col++) {
            row = col;
            while (row <= mtx_copy->rows && ELEM(mtx_copy, row, col) == 0) {
                row++;
            }
            if (row == mtx_copy->rows + 1) { /* oops all zero */
                *det = 0;
                matrixDestroy(mtx_copy);
                return 0;
            }
            if (row != col) { /* swap rows */
                for (j = 1; j <= mtx_copy->cols; j++) {
                    temp = ELEM(mtx_copy, row, j);              /*     temp <- found    */
                    ELEM(mtx_copy, row, j) = ELEM(mtx_copy, col, j); /*    found <- diagonal */
                    ELEM(mtx_copy, col, j) = temp;              /* diagonal <- temp     */
                }
                *det *= -1; /* must test if this works */
            }
            for (j = 1; j <= mtx_copy->cols; j++) { /* store diagonal */
                diagonal[j-1] = ELEM(mtx_copy, j, j);
            }

            for (i = col + 1; i <= mtx_copy->rows; i++) {
                if (ELEM(mtx_copy, i, col) == 0) { continue; }
                scalar = ELEM(mtx_copy, i, col) / diagonal[col - 1];
                for (j = 1; j <= mtx_copy->cols; j++) {
                    ELEM(mtx_copy, i, j) -= scalar * ELEM(mtx_copy, col, j);
                }
            }
        }

        for (i = 1; i <= mtx_copy->cols; i++) {
            *det *= ELEM(mtx_copy, i, i);
        }
    }

    matrixDestroy(mtx_copy);
    return 0;
}

/* Invertible matrix theorem
 * 1. There exists a square matrix C∈Rn×n such that AC=CA=I
 * 2. The columns of A are linearly independent
 * 3. The rows of A are linearly independent
 * 4. The columns of A span all of Rn
 * 5. The rows of A span all of Rn
 * 6. The rank of A is n
 * 7. The nullity of A is 0
 * 8. The linear transformation T(x):=Ax is one-to-one and onto.
 * 9. The equation Ax=0 has only the trivial solution x=0
 * 10. It is possible to use the row reduction algorithm to transform A into I
 * 11. There exists a sequence of elementary matrices E1,E2,…,Em such that E1E2…EmA=I
 * 12. |Det(A)| > 0
 */
int inverse(matrix * in, matrix * out) {
    matrix * adjoint_mtx;
    double det;

    if (!in || !out) { return -1; }
    if (in->rows != out->cols || in->cols != out->rows) { return -2; }

    adjoint_mtx = matrixCreate(in->rows, in->cols);
    if (adjoint(in, adjoint_mtx) == -3) { return -3; }
    
    determinant(in, &det);
    divide(det, adjoint_mtx, out);

    matrixDestroy(adjoint_mtx);
    return 0;
}

int isSquare(matrix * mtx) {
    return mtx && (mtx->rows == mtx->cols);
}

int isDiagonal(matrix * mtx) {
    int row, col;
    if (!isSquare(mtx)) { return 0; }
    for (col = 1; col <= mtx->cols; col++)
        for (row = 1; row <= mtx->rows; row++)
            /* if the element is not on the diagonal and not 0 */
            if (row != col && ELEM(mtx, row, col) != 0.0) {
                /* then the matrix is not diagonal */
                return 0;
            }

    return 1;
}

int isUpperTriangular(matrix * mtx) {
    int row, col;
    if (!isSquare(mtx)) { return 0; }
    /* looks at positions below the diagonal */
    for (col = 1; col <= mtx->cols; col++)
        for (row = col+1; row <= mtx->rows; row++) 
            if (ELEM(mtx, row, col) != 0.0) {
                return 0;
            }

    return 1;
}

int diagonal(matrix * v, matrix * mtx) {
    int row, col;
    if (!v || !mtx ||
        v->cols > 1 || v->rows != mtx->rows ||
        mtx->cols != mtx->rows) {
        return -1;
    }
    for (col = 1; col <= mtx->cols; col++)
        for (row = 1; row <= mtx->rows; row++)
            if (row == col) {
                ELEM(mtx, row, col) = ELEM(v, col, 1);
            } else {
                ELEM(mtx, row, col) = 0.0;
            }

    return 0;
}

#endif
