#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <curand.h>
#include <curand_kernel.h>
#include <stdio.h>
#include <cuda_runtime.h>

#include "params.h"

# define M_PI 3.14159265358979323846	


#define CUDA_CA LL(x) if (x != cudaSuccess) { \
        fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err));\
        exit(EXIT_FAILURE);}
#define CURAND_CALL(x) if((x)!=CURAND_STATUS_SUCCESS) { \
    printf("Error at %s:%d\n",__FILE__,__LINE__);\
    return;}

static __device__ __inline__ uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;  // Desplazamiento a la izquierda y XOR
    x ^= x >> 17;  // Desplazamiento a la derecha y XOR
    x ^= x << 5;   // Desplazamiento a la izquierda y XOR
    *state = x;    // Actualizar el estado
    return x;
}

__global__ void init_curand(curandState *states, unsigned long seed) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    curand_init(seed, tid, 0, &states[tid]);
}

__global__ void photon(float* heats, float* heats_squared, curandState* states) {
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    curandState state_d = states[tid];

    const float albedo = MU_S / (MU_S + MU_A);
    const float one_minus_albedo = 1.0f - albedo;
    const float one_minus_albedo_sq = one_minus_albedo * one_minus_albedo;
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    const float uint32_max = (float)UINT32_MAX;

    
    __shared__ float shared_heats[SHELLS];
    __shared__ float shared_heats_squared[SHELLS];

    if (threadIdx.x < SHELLS) {
        shared_heats[threadIdx.x] = 0.0f;
        shared_heats_squared[threadIdx.x] = 0.0f;
    }
    __syncthreads();

    for (size_t k=0; k<NUM_PHOTONS_PER_THREAD; ++k) {
        uint32_t state = curand_uniform(&state_d)* UINT32_MAX;
        
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float u = 0.0f, v = 0.0f, w = 1.0f;
        float weight = 1.0f;

        //float local_heats[SHELLS] = {0};
        //float local_heats_squared[SHELLS] = {0};

        for (;;) {
            float t = -__logf(xorshift32(&state) / uint32_max);
            x += t * u;
            y += t * v;
            z += t * w;

            unsigned int shell = sqrtf(x * x + y * y + z * z) * shells_per_mfp;
            shell = min(shell,SHELLS - 1);

            atomicAdd(&shared_heats[shell], one_minus_albedo * weight);
            //local_heats[shell] += one_minus_albedo * weight;
            atomicAdd(&shared_heats_squared[shell], one_minus_albedo_sq * weight* weight);
            //local_heats_squared[shell] += one_minus_albedo_sq * weight * weight;
            
            weight *= albedo;

            float xi1 = xorshift32(&state) / uint32_max;
            float xi2 = xorshift32(&state) / uint32_max;
            float phi = 2.0f * M_PI * xi1;
            float costheta = 2.0f * xi2 - 1.0f;
            float sintheta = __fsqrt_rn(1.0f - costheta * costheta);

            u = sintheta * __cosf(phi);
            v = sintheta * __sinf(phi);
            w = costheta;

            if (weight < 0.001f) {
                if (xorshift32(&state) / uint32_max > 0.1f)
                    break;
                weight /= 0.1f;
            }
        }

        states[tid] = state_d;
    }
    __syncthreads();
    // Accumulate results in shared memory
    if (threadIdx.x < SHELLS) {
        atomicAdd(&heats[threadIdx.x], shared_heats[threadIdx.x]);
        atomicAdd(&heats_squared[threadIdx.x],shared_heats_squared[threadIdx.x]);
    }
}
