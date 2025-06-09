/* Tiny Monte Carlo by Scott Prahl (http://omlc.ogi.edu)"
 * 1 W Point Source Heating in Infinite Isotropic Scattering Medium
 * http://omlc.ogi.edu/software/mc/tiny_mc.c
 *
 * Adaptado para CP2014, Nicolas Wolovick
 */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500 // M_PI
#endif

#include "params.h"
#include "photon.h"
#include "wtime.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define CUDA_CALL(x) err = x; \
    if (err != cudaSuccess) { \
    fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err));\
    exit(EXIT_FAILURE);}

char t1[] = "Tiny Monte Carlo by Scott Prahl (http://omlc.ogi.edu)";
char t2[] = "1 W Point Source Heating in Infinite Isotropic Scattering Medium";
char t3[] = "CPU version, adapted for PEAGPGPU by Gustavo Castellano"
            " and Nicolas Wolovick";


// global state, heat and heat square in each shell
static float heat[SHELLS];
static float heat2[SHELLS];


/***
 * Main matter
 ***/

int main(void)
{
    // heading
    printf("# %s\n# %s\n# %s\n", t1, t2, t3);
    printf("# Scattering = %8.3f/cm\n", MU_S);
    printf("# Absorption = %8.3f/cm\n", MU_A);
    printf("# Photons    = %8d\n#\n", PHOTONS);

    cudaError_t err;
    
    float *d_heat, *d_heat2;
    CUDA_CALL(cudaMalloc(&d_heat, SHELLS * sizeof(float)));
    CUDA_CALL(cudaMalloc(&d_heat2, SHELLS * sizeof(float)));

    dim3 grid(PHOTONS / BLOCK_SIZE+1);
    dim3 block(BLOCK_SIZE);

    // initialize states
    curandState *d_states;
    CUDA_CALL(cudaMalloc(&d_states, grid.x * block.x * sizeof(curandState)));

    init_curand<<<grid, block>>>(d_states, SEED);
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("Error al lanzar el kernel init_curand: %s\n", cudaGetErrorString(err));
    }
    CUDA_CALL(cudaDeviceSynchronize());

    // start timer
    double start = wtime();
    // simulation
    photon<<<grid,block>>>(d_heat, d_heat2, d_states);
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("Error al lanzar el kernel photon: %s\n", cudaGetErrorString(err));
    }
    CUDA_CALL(cudaDeviceSynchronize());
    // stop timer
    double end = wtime();
    assert(start <= end);
    double elapsed = end - start;

    CUDA_CALL(cudaMemcpy(heat, d_heat, SHELLS * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CALL(cudaMemcpy(heat2, d_heat2, SHELLS * sizeof(float), cudaMemcpyDeviceToHost));

    printf("# %lf seconds\n", elapsed);
    printf("# %lf K photons per second\n", 1e-3 * PHOTONS / elapsed);
    
    printf("# Radius\tHeat\n");
    printf("# [microns]\t[W/cm^3]\tError\n");
    float t = 4.0f * M_PI * powf(MICRONS_PER_SHELL, 3.0f) * PHOTONS / 1e12;
    
    for (unsigned int i = 0; i < SHELLS - 1; ++i) {
        printf("%6.0f\t%12.5f\t%12.5f\n", i * (float)MICRONS_PER_SHELL,
        heat[i] / t / (i * i + i + 1.0 / 3.0),
        sqrt(heat2[i] - heat[i] * heat[i] / PHOTONS) / t / (i * i + i + 1.0f / 3.0f));
    }
    printf("# extra\t%12.5f\n", heat[SHELLS - 1] / PHOTONS);
    
    cudaFree(d_states);
    cudaFree(d_heat);
    cudaFree(d_heat2);
    return 0;
}
