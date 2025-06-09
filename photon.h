#pragma once
#include <curand_kernel.h>

__global__ void init_curand(curandState *states, unsigned long seed);
__global__ void photon(float *heats, float *heats_squared, curandState *states);


