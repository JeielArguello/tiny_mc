#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>
#include <stdio.h>

#include "params.h"

/* static inline uint32_t xorshift32(uint32_t* restrict state) {
    uint32_t x = *state;
    x ^= x << 13;  // Desplazamiento a la izquierda y XOR
    x ^= x >> 17;  // Desplazamiento a la derecha y XOR
    x ^= x << 5;   // Desplazamiento a la izquierda y XOR
    *state = x;    // Actualizar el estado
    return x;
}
*/
/*
void photon(float* restrict heats, float* restrict heats_squared)
{
    const float albedo = MU_S / (MU_S + MU_A);
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    uint32_t state = (uint32_t)rand(); // Initialize RNG state

    // launch 
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float w = 1.0f;
    float weight = 1.0f;

    #pragma omp simd
    for (;;) {
        float t = -logf(xorshift32(&state) / (float)UINT32_MAX ); // move 
        x += t * u;
        y += t * v;
        z += t * w;

        unsigned int shell = sqrtf(x * x + y * y + z * z) * shells_per_mfp; // absorb 
        if (shell > SHELLS - 1) {
            shell = SHELLS - 1;
        }
        heats[shell] += (1.0f - albedo) * weight;
        heats_squared[shell] += (1.0f - albedo) * (1.0f - albedo) * weight * weight; // add up squares 
        weight *= albedo;

        // New direction, rejection method
        float xi1, xi2;
        do {
            xi1 = 2.0f * xorshift32(&state) / (float)UINT32_MAX - 1.0f;
            xi2 = 2.0f * xorshift32(&state) / (float)UINT32_MAX - 1.0f;
            t = xi1 * xi1 + xi2 * xi2;
        } while (1.0f < t);
        u = 2.0f * t - 1.0f;
        v = xi1 * sqrtf((1.0f - u * u) / t);
        w = xi2 * sqrtf((1.0f - u * u) / t);

        if (weight < 0.001f) { // roulette 
            if (xorshift32(&state) / (float)UINT32_MAX > 0.1f)
                break;
            weight /= 0.1f;
        }
    }
}
 */
static inline __m256 xorshift32_avx(__m256i* restrict state) {
    __m256i x = *state;
    x = _mm256_xor_si256(x, _mm256_slli_epi32(x, 13));  // Desplazamiento a la izquierda y XOR
    x = _mm256_xor_si256(x, _mm256_srli_epi32(x, 17));  // Desplazamiento a la derecha y XOR
    x = _mm256_xor_si256(x, _mm256_slli_epi32(x, 5));   // Desplazamiento a la izquierda y XOR
    *state = x;    // Actualizar el estado
    __m256i unsigned_x = _mm256_and_si256(x, _mm256_set1_epi32(0x7FFFFFFF));

    return _mm256_cvtepi32_ps(unsigned_x);
}

static __m256 log_vector(__m256 x) {
    // Almacenar los elementos del vector en un arreglo
    float elements[8];
    _mm256_storeu_ps(elements, x);

    // Calcular el logaritmo de cada elemento de manera escalar
    for (int i = 0; i < 8; ++i) {
        if(elements[i] <= 0.0f) {
            elements[i] = rand()/(float)UINT32_MAX;
        }
        elements[i] = logf(elements[i]);
    }

    // Cargar los resultados de nuevo en un vector __m256
    return _mm256_loadu_ps(elements);
}



void photon(float* restrict heats, float* restrict heats_squared){
    const float albedo = MU_S / (MU_S + MU_A);
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    __m256 shells_per_mfp_vec = _mm256_set1_ps(shells_per_mfp);
    // Tomo 8 semillas para los valores random
    __m256i state = _mm256_set_epi32(rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand()); // Initialize RNG state

    __m256 zero = _mm256_set1_ps(0.0f);
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 two = _mm256_set1_ps(2.0f);
    // Variables para 8 fotones
    __m256 x = zero;
    __m256 y = zero;
    __m256 z = zero;
    __m256 u = zero;
    __m256 v = zero;
    __m256 w = one;
    __m256 weight = one;

    // Valor INT_MAX en vector
    __m256 int_max = _mm256_set1_ps((float)UINT32_MAX);
    __m256 minus_one = _mm256_set1_ps(-1.0f);
    __m256 mask_vivo = minus_one;

    for (;;){
        __m256 log = log_vector(_mm256_div_ps(xorshift32_avx(&state), int_max));  
        __m256 t = _mm256_mul_ps(minus_one, log); /* move */
        

        x = _mm256_add_ps(x, _mm256_mul_ps(t, u));
        y = _mm256_add_ps(y, _mm256_mul_ps(t, v));
        z = _mm256_add_ps(z, _mm256_mul_ps(t, w));

        __m256 n = _mm256_add_ps(_mm256_mul_ps(x, x), _mm256_add_ps(_mm256_mul_ps(y, y), _mm256_mul_ps(z, z))); /* shell */
        __m256 shell_sqrt = _mm256_rsqrt_ps(n);
        __m256 shell = _mm256_mul_ps(_mm256_mul_ps(shell_sqrt,n), shells_per_mfp_vec); /* absorb */

        shell = _mm256_min_ps(shell, _mm256_set1_ps(SHELLS - 1)); // Asegurarse de que no exceda el límite
        __m256i shell_int = _mm256_cvttps_epi32(shell); 
        
        int indices[8];
        _mm256_storeu_si256((__m256i*)indices, shell_int);

        __m256 contribution_heats =_mm256_mul_ps(_mm256_set1_ps(1.0f - albedo), weight);
        contribution_heats = _mm256_and_ps(contribution_heats, mask_vivo);
        __m256 contribution_heats_squared = _mm256_mul_ps(_mm256_set1_ps(1.0f - albedo), _mm256_mul_ps(_mm256_set1_ps(1.0f - albedo), _mm256_mul_ps(weight, weight))); /* add up squares */
        contribution_heats_squared = _mm256_and_ps(contribution_heats_squared, mask_vivo); 



        float contribution_heats_index[8];
        _mm256_storeu_ps(contribution_heats_index, contribution_heats);
        float contribution_heats_squared_index[8];
        _mm256_storeu_ps(contribution_heats_squared_index, contribution_heats_squared);

        for (int i = 0; i < 8; i++) {
            
            int index = indices[i]; 
            heats[index] = heats[index] + contribution_heats_index[i]; /* add up heats */
            heats_squared[index] = heats_squared[index] + contribution_heats_squared_index[i]; /* add up squares */
        }
        
        weight = _mm256_mul_ps(weight, _mm256_set1_ps(albedo));        

        __m256 xi1, xi2;
        __m256 mask  = _mm256_cmp_ps( one , one, _CMP_EQ_OS);
      
        do{
            xi1 = _mm256_mul_ps(two, _mm256_div_ps(xorshift32_avx(&state), int_max));
            xi1 = _mm256_sub_ps(xi1, one);

            xi2 = _mm256_mul_ps(two, _mm256_div_ps(xorshift32_avx(&state), int_max));
            xi2 = _mm256_sub_ps(xi2, one);
            
            __m256 temp = _mm256_add_ps(_mm256_mul_ps(xi1,xi1), _mm256_mul_ps(xi2,xi2));
            
            t = _mm256_blendv_ps (t,temp, mask);
            mask = _mm256_cmp_ps(t, one, _CMP_LE_OQ);


        } while (_mm256_testc_ps(mask, minus_one ) != 0);

        u = _mm256_mul_ps(two, t);
        u = _mm256_sub_ps(u, one);

        __m256 rest_u =_mm256_div_ps( _mm256_sub_ps(one, _mm256_mul_ps(u, u)), t);
                

        v =_mm256_mul_ps(xi1, _mm256_mul_ps( _mm256_rsqrt_ps(rest_u),rest_u));
        w =_mm256_mul_ps(xi2, _mm256_mul_ps( _mm256_rsqrt_ps(rest_u),rest_u));
    
        // Ruleta Rusa
        __m256 mask_weight = _mm256_cmp_ps(weight, _mm256_set1_ps(0.001f), _CMP_LT_OQ);
        if (_mm256_movemask_ps(mask_weight) != 0x00) {
            
            
            __m256 rand_vals_roulette = _mm256_div_ps(xorshift32_avx(&state), int_max);
            __m256 mask_eliminado = _mm256_cmp_ps(rand_vals_roulette, _mm256_set1_ps(0.1f), _CMP_GT_OQ);
            mask_vivo = _mm256_andnot_ps(mask_eliminado, mask_vivo);


            if (_mm256_movemask_ps(mask_vivo) == 0x00) {
                break;
            }
           
           
            __m256 updated_weight = _mm256_div_ps(weight, _mm256_set1_ps(0.1f));
            weight = _mm256_blendv_ps(weight, updated_weight, mask_weight);

        }
    }

}

/* 
if (weight < 0.001f) { // roulette 
    if (xorshift32(&state) / (float)UINT32_MAX > 0.1f)
        break;
    weight /= 0.1f;
} */