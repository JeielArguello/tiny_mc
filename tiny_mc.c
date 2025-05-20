/* Tiny Monte Carlo by Scott Prahl (http://omlc.ogi.edu)"
 * 1 W Point Source Heating in Infinite Isotropic Scattering Medium
 * http://omlc.ogi.edu/software/mc/tiny_mc.c
 *
 * Adaptado para CP2014, Nicolas Wolovick
 */

#define _XOPEN_SOURCE 500 // M_PI


#include "params.h"
#include "photon.h"
#include "wtime.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>

char t1[] = "Tiny Monte Carlo by Scott Prahl (http://omlc.ogi.edu)";
char t2[] = "1 W Point Source Heating in Infinite Isotropic Scattering Medium";
char t3[] = "CPU version, adapted for PEAGPGPU by Gustavo Castellano"
            " and Nicolas Wolovick";


// global state, heat and heat square in each shell
static float heat[SHELLS * FLOATS_PER_CACHE_LINE];
static float heat2[SHELLS * FLOATS_PER_CACHE_LINE];


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

    // start timer
    double start = wtime();
    // simulation
    #pragma omp parallel
    {
        unsigned int seed = SEED + omp_get_thread_num();
        #pragma omp for simd reduction(+:heat[0:SHELLS*FLOATS_PER_CACHE_LINE], heat2[0:SHELLS*FLOATS_PER_CACHE_LINE])
        for (unsigned int i = 0; i < PHOTONS; i = i + 8) {
            photon(heat, heat2, &seed);
        }
    }
    // stop timer
    double end = wtime();
    assert(start <= end);
    double elapsed = end - start;

    printf("# %lf seconds\n", elapsed);
    printf("# %lf K photons per second\n", 1e-3 * PHOTONS / elapsed);

    printf("# Radius\tHeat\n");
    printf("# [microns]\t[W/cm^3]\tError\n");
    float t = 4.0f * M_PI * powf(MICRONS_PER_SHELL, 3.0f) * PHOTONS / 1e12;

   for (unsigned int i = 0; i < SHELLS - 1; ++i) {
        printf("%6.0f\t%12.5f\t%12.5f\n", i * (float)MICRONS_PER_SHELL,
               heat[i*FLOATS_PER_CACHE_LINE] / t / (i * i + i + 1.0 / 3.0),
               sqrt(heat2[i*FLOATS_PER_CACHE_LINE] - heat[i*FLOATS_PER_CACHE_LINE] * heat[i*FLOATS_PER_CACHE_LINE] / PHOTONS) / t / (i * i + i + 1.0f / 3.0f));
    }
    printf("# extra\t%12.5f\n", heat[SHELLS - FLOATS_PER_CACHE_LINE-1] / PHOTONS);

    return 0;
}
