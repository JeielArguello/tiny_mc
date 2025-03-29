#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "params.h"

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;  // Desplazamiento a la izquierda y XOR
    x ^= x >> 17;  // Desplazamiento a la derecha y XOR
    x ^= x << 5;   // Desplazamiento a la izquierda y XOR
    *state = x;    // Actualizar el estado
    return x;
}

void photon(float* heats, float* heats_squared)
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

        /* New direction, rejection method */
        float xi1, xi2;
        do {
            xi1 = 2.0f * xorshift32(&state) / (float)UINT32_MAX - 1.0f;
            xi2 = 2.0f * xorshift32(&state) / (float)UINT32_MAX - 1.0f;
            t = xi1 * xi1 + xi2 * xi2;
        } while (1.0f < t);
        u = 2.0f * t - 1.0f;
        v = xi1 * sqrtf((1.0f - u * u) / t);
        w = xi2 * sqrtf((1.0f - u * u) / t);

        if (weight < 0.001f) { /* roulette */
            if (xorshift32(&state) / (float)UINT32_MAX > 0.1f)
                break;
            weight /= 0.1f;
        }
    }
}
