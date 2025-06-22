#pragma once
#include <curand_kernel.h>


__global__ void photon(float *heats, float *heats_squared, unsigned long seed);


