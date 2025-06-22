#include <cuda_runtime.h>
#include <curand.h>
#include <curand_kernel.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "params.h"

#define M_PI 3.14159265358979323846


#define CUDA_CA                                                       \
    LL(x)                                                             \
    if (x != cudaSuccess)                                             \
    {                                                                 \
        fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err)); \
        exit(EXIT_FAILURE);                                           \
    }
#define CURAND_CALL(x)                                  \
    if ((x) != CURAND_STATUS_SUCCESS) {                 \
        printf("Error at %s:%d\n", __FILE__, __LINE__); \
        return;                                         \
    }

static __device__ __inline__ uint32_t xorshift32(uint32_t* state)
{
    uint32_t x = *state;
    x ^= x << 13; // Desplazamiento a la izquierda y XOR
    x ^= x >> 17; // Desplazamiento a la derecha y XOR
    x ^= x << 5; // Desplazamiento a la izquierda y XOR
    *state = x; // Actualizar el estado
    return x;
}


__global__ void photon(float* __restrict__ heats, float* __restrict__ heats_squared, unsigned long seed)
{
    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    //__shared__ curandState state_d;

    // if (threadIdx.x == 0) {
    //   curand_init(seed,blockIdx.x,0, &state_d);
    //}
    //__syncthreads();

    const float albedo = MU_S / (MU_S + MU_A);
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    const float uint32_max = (float)UINT32_MAX;


    __shared__ float shared_heats[SHELLS];
    __shared__ float shared_heats_squared[SHELLS];

    if (threadIdx.x < SHELLS) {
        shared_heats[threadIdx.x] = 0.0f;
        shared_heats_squared[threadIdx.x] = 0.0f;
    }
    __syncthreads();

    curandState local_state;
    curand_init(seed, tid, 0, &local_state);
    // skipahead_sequence((tid+1) *(threadIdx.x+1)*NUM_PHOTONS_PER_THREAD*seed, &local_state);


    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float dir[3] = { 0.0f, 0.0f, 1.0f };
    float weight = 1.0f;
    unsigned int photon_count = 0;
    while (photon_count < NUM_PHOTONS_PER_THREAD) {

        float t = -__logf(curand_uniform(&local_state));
        for (size_t i = 0; i < 3; i++) {
            pos[i] += t * dir[i];
        }

        unsigned int shell = __fsqrt_rn(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]) * shells_per_mfp;
        shell = min(shell, SHELLS - 1);

        float absorption = (1.0f - albedo) * weight;
        atomicAdd(&shared_heats[shell], absorption);
        // local_heats[shell] += one_minus_albedo * weight;
        atomicAdd(&shared_heats_squared[shell], absorption * absorption);
        // local_heats_squared[shell] += one_minus_albedo_sq * weight * weight;

        weight *= albedo;

        float xi1 = curand_uniform(&local_state);
        float xi2 = curand_uniform(&local_state);
        float phi = 2.0f * M_PI * xi1;
        float costheta = 2.0f * xi2 - 1.0f;
        float sintheta = __fsqrt_rn(1.0f - costheta * costheta);

        dir[0] = sintheta * __cosf(phi);
        dir[1] = sintheta * __sinf(phi);
        dir[2] = costheta;

        if (weight < 0.001f) {
            if (curand_uniform(&local_state) > 0.1f) {
                pos[0] = 0.0f;
                pos[1] = 0.0f;
                pos[2] = 0.0f;
                dir[0] = 0.0f;
                dir[1] = 0.0f;
                dir[2] = 1.0f;
                weight = 1.0f;
                photon_count += 1;
            } else {
                weight /= 0.1f;
            }
        }
    }

    __syncthreads();
    // Accumulate results in shared memory
    if (threadIdx.x < SHELLS) {
        atomicAdd(&heats[threadIdx.x], shared_heats[threadIdx.x]);
        atomicAdd(&heats_squared[threadIdx.x], shared_heats_squared[threadIdx.x]);
    }
}
