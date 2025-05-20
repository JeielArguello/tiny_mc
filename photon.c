#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <stdio.h>

#include "params.h"


# define M_PI 3.14159265358979323846	

#ifdef _ICX    // Si se usa el compilador Intel, utiliza _mm256_log_ps
    #define LOG_FUNCTION(x) _mm256_log_ps(x)
#else
    // Si no es Intel, utiliza log_vector
    #define LOG_FUNCTION(x) log_vector(x)
#endif

#ifdef _ICX    // Si se usa el compilador Intel, utiliza _mm256_cos_ps
    #define COS_FUNCTION(x) _mm256_cos_ps(x)
#else
    // Si no es Intel, utiliza cos_vector
    #define COS_FUNCTION(x) cos_vector(x)
#endif

#ifdef _ICX    // Si se usa el compilador Intel, utiliza _mm256_sin_ps
    #define SIN_FUNCTION(x) _mm256_sin_ps(x)
#else
    // Si no es Intel, utiliza cos_vector
    #define SIN_FUNCTION(x) sin_vector(x)
#endif


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
            elements[i] = rand()/(float)RAND_MAX;
        }
        elements[i] = logf(elements[i]);
    }

    // Cargar los resultados de nuevo en un vector __m256
    return _mm256_loadu_ps(elements);
}

static __m256 cos_vector(__m256 x) {
    // Almacenar los elementos del vector en un arreglo
    float elements[8];
    _mm256_storeu_ps(elements, x);

    // Calcular el coseno de cada elemento de manera escalar
    for (int i = 0; i < 8; ++i) {
        elements[i] = cosf(elements[i]);
    }

    // Cargar los resultados de nuevo en un vector __m256
    return _mm256_loadu_ps(elements);
}

static __m256 sin_vector(__m256 x) {
    // Almacenar los elementos del vector en un arreglo
    float elements[8];
    _mm256_storeu_ps(elements, x);

    // Calcular el seno de cada elemento de manera escalar
    for (int i = 0; i < 8; ++i) {
        elements[i] = sinf(elements[i]);
    }

    // Cargar los resultados de nuevo en un vector __m256
    return _mm256_loadu_ps(elements);
}



void photon(float* restrict heats, float* restrict heats_squared){
    const float albedo = MU_S / (MU_S + MU_A);
    const float shells_per_mfp = 1e4 / MICRONS_PER_SHELL / (MU_A + MU_S);
    __m256 shells_per_mfp_vec = _mm256_set1_ps(shells_per_mfp);
    
    float local_heats[SHELLS] = {0};
    float local_heats_sq[SHELLS] = {0};

    // Tomo 8 semillas para los valores random
    __m256i state = _mm256_set_epi32(rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand()); // Initialize RNG state

    // vectores que utilizo seguido
    __m256 zero = _mm256_set1_ps(0.0f);
    __m256 zero_one = _mm256_set1_ps(0.1f);
    __m256 zero_zero_one = _mm256_set1_ps(0.001f);
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 two = _mm256_set1_ps(2.0f);
    __m256 int_max = _mm256_set1_ps((float)INT32_MAX);
    __m256 minus_one = _mm256_set1_ps(-1.0f);
    __m256 shell_max = _mm256_set1_ps(SHELLS - 1);
    __m256 albedo_vec = _mm256_set1_ps(albedo);
    __m256 minus_albedo_vec = _mm256_set1_ps(1.0f - albedo);
    __m256 pi_vec = _mm256_set1_ps(M_PI);

    // Variables para 8 fotones
    __m256 x = zero;
    __m256 y = zero;
    __m256 z = zero;
    __m256 u = zero;
    __m256 v = zero;
    __m256 w = one;
    __m256 weight = one;
    
    // Vector que dice que foton sigue vivo
    __m256 mask_vivo = minus_one;

    for (;;){
        // Calculo el tiempo de absorcion
        // se utiliza LOG_FUNCTION para el caso de Intel
        __m256 log = LOG_FUNCTION(_mm256_div_ps(xorshift32_avx(&state), int_max));  
        __m256 t = _mm256_mul_ps(minus_one, log); /* move */
        

        x = _mm256_add_ps(x, _mm256_mul_ps(t, u));
        y = _mm256_add_ps(y, _mm256_mul_ps(t, v));
        z = _mm256_add_ps(z, _mm256_mul_ps(t, w));

        // calculo shell usando la reciproca de la raiz cuadrada
        __m256 n = _mm256_add_ps(_mm256_mul_ps(x, x), _mm256_add_ps(_mm256_mul_ps(y, y), _mm256_mul_ps(z, z))); /* shell */
        __m256 shell_sqrt = _mm256_rsqrt_ps(n);
        __m256 shell = _mm256_mul_ps(_mm256_mul_ps(shell_sqrt,n), shells_per_mfp_vec); /* absorb */

        shell = _mm256_min_ps(shell, shell_max); // Asegurarse de que no exceda el límite
        // convierto a entero
        __m256i shell_int = _mm256_cvttps_epi32(shell); 
        
        // tomo los indices del vector shells
        int indices[8];
        _mm256_storeu_si256((__m256i*)indices, shell_int);

        // Calculo el peso de cada foton
        __m256 contribution_heats =_mm256_mul_ps(minus_albedo_vec, weight);
        contribution_heats = _mm256_and_ps(contribution_heats, mask_vivo);
        __m256 contribution_heats_squared = _mm256_mul_ps(minus_albedo_vec, _mm256_mul_ps(minus_albedo_vec, _mm256_mul_ps(weight, weight))); /* add up squares */
        contribution_heats_squared = _mm256_and_ps(contribution_heats_squared, mask_vivo); 


        // guardo la contribucion de cada foton
        float contribution_heats_index[8];
        _mm256_storeu_ps(contribution_heats_index, contribution_heats);
        float contribution_heats_squared_index[8];
        _mm256_storeu_ps(contribution_heats_squared_index, contribution_heats_squared);

        // sumo la contribucion de cada foton a su shell correspondiente
        for (int i = 0; i < 8; i++) {
            
            int index = indices[i]; 
            local_heats[index] = local_heats[index] + contribution_heats_index[i]; /* add up heats */
            local_heats_sq[index] = local_heats_sq[index] + contribution_heats_squared_index[i]; /* add up squares */
        }
        
        // Calculo el nuevo peso
        weight = _mm256_mul_ps(weight, albedo_vec);  
        
        // New direction, polar coordinates (isotropic 3D)
        __m256 xi1 = _mm256_div_ps(xorshift32_avx(&state), int_max); // Uniform in [0,1)
        __m256 xi2 = _mm256_div_ps(xorshift32_avx(&state), int_max); // Uniform in [0,1)
        
        __m256 phi = _mm256_mul_ps(_mm256_mul_ps(two, pi_vec), xi1); 
        __m256 costheta = _mm256_sub_ps(_mm256_mul_ps(two, xi2), one); // cos(θ) in [-1, 1]
        __m256 sinres = _mm256_sub_ps(one, _mm256_mul_ps(costheta, costheta));
        __m256 sintheta = _mm256_mul_ps(_mm256_rsqrt_ps(sinres),sinres); // sin(θ) in [0, 1]
        
        u = _mm256_mul_ps(sintheta, COS_FUNCTION(phi));  // x
        v = _mm256_mul_ps(sintheta, SIN_FUNCTION(phi));  // y
        w = costheta;              // z

        // Ruleta Rusa
        // si algun foton tiene un peso menor a 0.001f deberia entrar al if
        __m256 mask_weight = _mm256_cmp_ps(weight, zero_zero_one, _CMP_LT_OQ);
       
        // la condicion != 0x00 significa que al menos un foton tiene peso menor a 0.001f
        if (_mm256_movemask_ps(mask_weight) != 0x00) {
            
            //hago la ruleta rusa
            __m256 rand_vals_roulette = _mm256_div_ps(xorshift32_avx(&state), int_max);
            __m256 roulette = _mm256_cmp_ps(rand_vals_roulette, zero_one, _CMP_GT_OQ);
            
            
            // elimino los fotones que no pasaron la ruleta rusa y que tienen peso menor a 0.001f
            __m256 mask_eliminado = _mm256_and_ps(roulette, mask_weight);
            mask_vivo = _mm256_andnot_ps(mask_eliminado, mask_vivo);

            // si la mascara es cero significa que todos los fotones fueron eliminados
            if (_mm256_movemask_ps(mask_vivo) == 0x00) {
                break;
            }
           
           // actualizo el peso de los fotones que no fueron eliminados
            __m256 updated_weight = _mm256_div_ps(weight, zero_one);
            weight = _mm256_blendv_ps(weight, updated_weight, mask_weight);

        }
    }

    for (int i = 0; i < SHELLS; ++i) {
        heats[i*FLOATS_PER_CACHE_LINE]        += local_heats[i];
        heats_squared[i*FLOATS_PER_CACHE_LINE] += local_heats_sq[i];
    }

}

