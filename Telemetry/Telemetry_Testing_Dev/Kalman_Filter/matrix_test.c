#include "matrix.h"

int main(void)
{
    /* testing matrix library */
    matrix * A, * Ac, * B, * c, * d, * M, * Mi, * ct, * mdp;
    double dp;

    A = matrixCreate(3, 3);
    setElement(A, 1, 1, 1.0);
    setElement(A, 1, 2, .25);
    setElement(A, 1, 3, -.1);
    setElement(A, 2, 2, .4);
    setElement(A, 2, 3, .3);
    setElement(A, 3, 2, .1);
    setElement(A, 3, 3, -.3);
    printf("Matrix A:\n");
    matrixPrint(A);

    Ac = matrixCopy(A);
    printf("\nCopy of A:\n");
    matrixPrint(Ac);

    B = matrixCreate(3, 3);
    setElement(B, 1, 1, .5);
    setElement(B, 2, 2, 2.0);
    setElement(B, 3, 3, 1.0);
    printf("\nMatrix B:\n");
    matrixPrint(B);

    c = matrixCreate(3, 1);
    setElement(c, 1, 1, 1.0);
    setElement(c, 3, 1, 1.0);
    printf("\nVector c:\n");
    matrixPrint(c);

    d = matrixCreate(3, 1);
    setElement(d, 2, 1, 1.0);
    setElement(d, 3, 1, 1.0);
    printf("\nVector d:\n");
    matrixPrint(d);

    M = matrixCreate(3, 3);
    transpose(A, M);
    printf("\nA':\n");
    matrixPrint(M);

    ct = matrixCreate(1, 3);
    transpose(c, ct);
    printf("\nc':\n");
    matrixPrint(ct);

    sum(A, B, M);
    Mi = matrixCopy(M);
    printf("\nA + B:\n");
    matrixPrint(M);
    inverse(M, Mi);
    printf("\nM^-1:\n");
    matrixPrint(Mi);

    product(A, B, M);
    printf("\nA * B:\n");
    matrixPrint(M);

    mdp = matrixCreate(1, 1);
    product(ct, d, mdp);
    getElement(mdp, 1, 1, &dp);
    printf("\nDot product (1): %.2f\n", dp);

    dotProduct(c, d, &dp);
    printf("\nDot product (2): %.2f\n", dp);

    product(A, c, d);
    printf("\nA * c:\n");
    matrixPrint(d);

    printf("\nisUpperTriangular(A): %d"
           "\nisUpperTriangular(B): %d"
           "\nisDiagonal(A): %d"
           "\nisDiagonal(B): %d\n",
           isUpperTriangular(A),
           isUpperTriangular(B),
           isDiagonal(A),
           isDiagonal(B));

    identity(A);
    printf("\nIdentity:\n");
    matrixPrint(A);

    diagonal(c, A);
    printf("\nDiagonal from c:\n");
    matrixPrint(A);

    matrixDestroy(A);
    matrixDestroy(Ac);
    matrixDestroy(B);
    matrixDestroy(c);
    matrixDestroy(d);
    matrixDestroy(M);
    matrixDestroy(Mi);
    matrixDestroy(ct);
    matrixDestroy(mdp);

    return 0;
}
