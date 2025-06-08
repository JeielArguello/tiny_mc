#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "params.h"

# define M_PI 3.14159265358979323846	

static inline uint32_t xorshift32(uint32_t* restrict state) {
    uint32_t x = *state;
    x ^= x << 13;  // Desplazamiento a la izquierda y XOR
    x ^= x >> 17;  // Desplazamiento a la derecha y XOR
    x ^= x << 5;   // Desplazamiento a la izquierda y XOR
    *state = x;    // Actualizar el estado
    return x;
}

void photon(float* restrict heats, float* restrict heats_squared)
{
    const float albedo = MU_S / (MU_S + MU_A);
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    uint32_t state = (uint32_t)rand(); // Initialize RNG state

    /* launch */
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float w = 1.0f;
    float weight = 1.0f;

    #pragma omp simd
    for (;;) {
        float t = -logf(xorshift32(&state) / (float)UINT32_MAX ); /* move */
        x += t * u;
        y += t * v;
        z += t * w;

        unsigned int shell = sqrtf(x * x + y * y + z * z) * shells_per_mfp; /* absorb */
        if (shell > SHELLS - 1) {
            shell = SHELLS - 1;
        }
        heats[shell] += (1.0f - albedo) * weight;
        heats_squared[shell] += (1.0f - albedo) * (1.0f - albedo) * weight * weight; /* add up squares */
        weight *= albedo;

        // New direction, polar coordinates (isotropic 3D)
        float xi1 = xorshift32(&state) / (float)UINT32_MAX;          // Uniform in [0,1)
        float xi2 = xorshift32(&state) / (float)UINT32_MAX;          // Uniform in [0,1)

        float phi = 2.0f * M_PI * xi1;                               // Azimuthal angle in [0, 2π)
        float costheta = 2.0f * xi2 - 1.0f;                          // cos(θ) in [-1, 1]
        float sintheta = sqrtf(1.0f - costheta * costheta);

        u = sintheta * cosf(phi);  // x
        v = sintheta * sinf(phi);  // y
        w = costheta;              // z

        if (weight < 0.001f) { /* roulette */
            if (xorshift32(&state) / (float)UINT32_MAX > 0.1f)
                break;
            weight /= 0.1f;
        }
    }
}
