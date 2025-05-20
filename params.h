#pragma once

#include <time.h> // time

#ifndef SHELLS
#define SHELLS 101 // discretization level
#endif

#ifndef PHOTONS
#define PHOTONS 32768 // 32K photons
#endif

#ifndef MU_A
#define MU_A 2.0f // Absorption Coefficient in 1/cm !!non-zero!!
#endif

#ifndef MU_S
#define MU_S 20.0f // Reduced Scattering Coefficient in 1/cm
#endif

#ifndef MICRONS_PER_SHELL
#define MICRONS_PER_SHELL 50 // Thickness of spherical shells in microns
#endif

#ifndef SEED
#define SEED (time(NULL)) // random seed
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64 // cache line size in bytes
#endif

#ifndef FLOATS_PER_CACHE_LINE
#define FLOATS_PER_CACHE_LINE (CACHE_LINE_SIZE / sizeof(float))
#endif


