#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "pcg.h"

double mean(double* arr, int size) {
    int i;
    double sum = 0;
    double mean;

    for (i = 0; i < size; i++) {
        sum += arr[i];
    }
    mean = sum / size;

    return mean;
}

double std_dev(double* arr, int size, double mean) {
    int i;
    double sum = 0;
    double std_dev, base;

    for (i = 0; i < size; i++) {
        base = (double)arr[i] - mean;
        sum += base * base;
    }

    sum /= size;
    std_dev = sqrt(sum);

    return std_dev;
}

int main(void) {
    pcg32_random_t rng;
    double nums[1024] = {0};
    int samples = 1024;
    double avg, dev;
    int i;

    /* Initialize the random number generator */
    rng.state = (uint64_t)time(0) ^ (uintptr_t)&rng ^ (uint64_t)clock();
    rng.inc = ((uint64_t)time(0) << 32 | (uint64_t)rand()) | 1; /* Make sure inc is odd */
    

    for (i=0; i<samples; i++) {
        nums[i] = pcg32_normal_r(&rng);
        /* printf("sample %d: %.4f\n", i, nums[i]); */
    }

    avg = mean(nums, 1024);
    dev = std_dev(nums, 1024, avg);

    printf("mean: %.4f\n", avg);
    printf("std dev: %.4f\n", dev);

    return 0;
}
