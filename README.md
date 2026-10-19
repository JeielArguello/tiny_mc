# Tiny Monte Carlo

- [Página en Wikipedia sobre el problema](https://en.wikipedia.org/wiki/Monte_Carlo_method_for_photon_transport)
- [Código original](https://omlc.org/software/mc/) de [Scott Prahl](https://omlc.org/~prahl/)

## Descripción del proyecto

`tiny_mc` simula transporte de fotones en un medio homogéneo con dispersión y absorción isotrópica mediante Monte Carlo. El programa modela trayectorias de muchos fotones independientes, acumula energía absorbida por capas esféricas (`SHELLS`) y reporta rendimiento en tiempo de ejecución y fotones/segundo.

## Algoritmo (resumen)

Para cada fotón:

1. Inicializa posición, dirección y peso.
2. Muestra la longitud libre de camino con una variable aleatoria exponencial.
3. Actualiza posición y determina la capa radial (`shell`).
4. Acumula energía absorbida y su cuadrado (`heat`, `heat2`) para estimar error.
5. Actualiza el peso del fotón según el albedo.
6. Recalcula dirección aleatoria isotrópica.
7. Aplica *roulette* cuando el peso es pequeño para terminar o renormalizar el fotón.

## Formato de salida

La ejecución de `./headless` imprime:

- Encabezado de parámetros físicos (`MU_S`, `MU_A`, `PHOTONS`).
- Tiempo total (`# X seconds`).
- Rendimiento (`# X K photons per second`).
- Tabla radial:
  - `Radius [microns]`
  - `Heat [W/cm^3]`
  - `Error`
- Fila final `# extra` para energía fuera del último *shell*.

## Requisitos

### Base CPU (ramas `main`, `lab2`, `codigo-optimizado`)
- Compilador C (`gcc` o `clang`; en scripts también `icx`).
- `make`.
- `libm` (normalmente disponible por defecto).
- `perf` para mediciones de microarquitectura.

### SIMD AVX2 (`lab2-SIMD`)
- CPU con AVX2.
- Compilador con intrínsecos AVX2 (se usa `icx` en la rama; también hay rutas condicionales para otros compiladores).

### OpenMP (`lab3`)
- Compilador con soporte OpenMP (`-fopenmp` o `-qopenmp`).

### CUDA (`lab4`)
- GPU NVIDIA compatible.
- CUDA Toolkit y `nvcc`.
- Driver CUDA funcional.

> Nota: `lab4` depende de hardware/driver CUDA y no puede validarse en equipos sin GPU NVIDIA.

## Compilación y ejecución

### `main` (base C)
```bash
git checkout main
make clean && make headless
./headless
```

### `lab2` (xorshift32 + cambios para vectorización)
```bash
git checkout lab2
make clean && make headless
./headless
```

### `codigo-optimizado` (xorshift32 + benchmarking ampliado)
```bash
git checkout codigo-optimizado
make clean && make headless
./headless
```

### `lab2-SIMD` (AVX2 explícito)
```bash
git checkout lab2-SIMD
make clean && make CC=icx EXTRA_CFLAGS="-O3 -march=native -funroll-loops" headless
./headless
```

### `lab3` (OpenMP)
```bash
git checkout lab3
export OMP_NUM_THREADS=4
make clean && make CC=icx CPPFLAGS="-qopenmp" EXTRA_CFLAGS="-O3" TINY_LDFLAGS="-lm -qopenmp" headless
./headless
```

### `lab4` (CUDA)
```bash
git checkout lab4
make clean && make headless
./headless
```

## Benchmarking y comparación reproducible

### Reglas para comparar de forma justa

1. Misma máquina y estado térmico similar.
2. Mismo número de fotones (`PHOTONS`) para todas las variantes comparadas.
3. Varias repeticiones por configuración (mínimo 5 recomendado).
4. Reportar promedio y desviación estándar.
5. Comparar contra una misma base (por ejemplo, `main` con flags definidas).

### Scripts disponibles por rama

- `script-comp.sh`:
  - `main`, `lab2`, `codigo-optimizado`: barrido de compilador/flags con métricas `perf`.
  - `lab2-SIMD`, `lab3`: añade columna de fotones/segundo.
- `tam_vs_hilos.sh` en `lab3`: barrido de tamaño de problema vs hilos OpenMP.
- `tam_vs_block.sh` en `lab4`: barrido de tamaño de bloque y fotones por hilo CUDA.
- `compare.py` en `lab2-SIMD` y `lab3`: comparación de CSV generados.

Ejemplos:
```bash
# benchmark base / compiladores (rama correspondiente)
./script-comp.sh

# OpenMP: tamaño vs hilos
git checkout lab3
./tam_vs_hilos.sh

# CUDA: tamaño vs bloque / fotones por hilo
git checkout lab4
./tam_vs_block.sh
```

## Evolución por rama (respecto a `main`)

| Rama | Objetivo | Cambios principales | Impacto esperado por diseño | Limitaciones / consideraciones |
|---|---|---|---|---|
| `main` | Implementación base Monte Carlo en CPU | Código C escalar, `rand()`, acumulación por *shells* | Línea base de corrección y rendimiento | Sirve como referencia; no explota paralelismo SIMD/hilos/GPU |
| `lab2` | Preparar mejor desempeño en CPU | Reemplazo de `rand()` por `xorshift32`, cambio de dirección a coordenadas polares isotrópicas, pragmas SIMD, script de benchmarking con compiladores/flags y `perf` | Menor costo del RNG y mayor oportunidad de vectorización automática | La presencia de pragmas no garantiza vectorización efectiva en todos los compiladores |
| `lab2-SIMD` | Vectorización explícita AVX2 | Procesamiento de 8 fotones con intrínsecos, funciones vectoriales auxiliares, medición de fotones/segundo | Mayor *throughput* SIMD en CPU AVX2 | Existen rutas auxiliares escalares para `log`, `sin` y `cos`; la ganancia debe validarse experimentalmente y con verificación numérica |
| `lab3` | Paralelización multinúcleo | OpenMP (`parallel`, `for simd`, `reduction`), semillas por hilo, padding para reducir *false sharing*/*cache contention*, scripts de hilos/rendimiento | Escalado por núcleos y menor contención de memoria | La escalabilidad depende de CPU, scheduler, afinidad e implementación del compilador; validar corrección numérica |
| `lab4` | Port a GPU con CUDA | Reescritura a `.cu`, kernels GPU, `curand`, memoria compartida y `atomicAdd`, parámetros de bloque/fotones por hilo, medición CPU/GPU | Aumentar fotones/segundo en hardware GPU adecuado | Requiere CUDA/GPU NVIDIA; resultados dependen del hardware; posibles diferencias de precisión por funciones rápidas (`__logf`, `__sinf`, `__cosf`) y orden de atomics |
| `codigo-optimizado` | Optimizar versión CPU sin migrar a CUDA/OpenMP | `xorshift32` y script de benchmarking ampliado (GCC/Clang/ICX, flags y métricas `perf`) | Mejor exploración de configuraciones de compilación y posible mejora de throughput | El aumento real depende del compilador, flags y microarquitectura; requiere ejecutar benchmarks |

## Metodología de rendimiento

Métricas recomendadas:

- `task-clock`: tiempo efectivo de CPU consumido.
- `cycles`: ciclos ejecutados.
- `instructions`: instrucciones retiradas.
- `branch-misses`: fallos de predicción de salto.
- `seconds` (tiempo total): latencia de punta a punta.
- `photons/s`: *throughput* de la simulación.

Fórmulas:

- `speedup = tiempo_base / tiempo_variante`
- `% mejora = ((tiempo_base - tiempo_variante) / tiempo_base) * 100`
- `throughput_speedup = (fotones_s_variante / fotones_s_base)`

### Plantilla de resultados (completar con datos medidos)

> Este repositorio no versiona una batería completa de resultados reproducibles para afirmar cifras globales de speedup entre todas las ramas. Usar la siguiente plantilla y completarla ejecutando `script-comp.sh`, `tam_vs_hilos.sh`, `tam_vs_block.sh` y/o `compare.py` en la rama correspondiente.

| Rama | Configuración (compilador/flags/hilos/bloque) | Repeticiones | Tiempo promedio (s) | Desv. estándar (s) | Photons/s promedio | Speedup vs base |
|---|---|---:|---:|---:|---:|---:|
| `main` (base) | `gcc -O2` |  |  |  |  | 1.00 |
| `lab2` |  |  |  |  |  |  |
| `lab2-SIMD` |  |  |  |  |  |  |
| `lab3` |  |  |  |  |  |  |
| `lab4` |  |  |  |  |  |  |
| `codigo-optimizado` |  |  |  |  |  |  |

## Mejoras esperadas vs mejoras medidas

- **Esperadas por diseño**: cambios de RNG, vectorización, OpenMP, CUDA, tuning de compilador.
- **Medidas**: únicamente lo obtenido al ejecutar los scripts en un entorno controlado.

No debe presentarse ninguna mejora como confirmada sin evidencia experimental reproducible. En particular, en `lab2-SIMD` y `lab3` la presencia de pragmas/intrínsecos no implica automáticamente mejora neta o corrección numérica sin validación.

## Observaciones del estado actual

- `lab2-SIMD`, `lab3` y `lab4` contienen implementaciones vectoriales/paralelas, pero su corrección, compilación y rendimiento deben verificarse en el compilador/hardware objetivo.
- `lab4` requiere CUDA y GPU NVIDIA; además, por funciones rápidas y atomics pueden aparecer diferencias numéricas respecto de CPU escalar.

## Referencias rápidas a scripts por rama

- `main`: [`script-comp.sh`](https://github.com/JeielArguello/tiny_mc/blob/main/script-comp.sh)
- `lab2`: [`script-comp.sh`](https://github.com/JeielArguello/tiny_mc/blob/lab2/script-comp.sh)
- `lab2-SIMD`: [`script-comp.sh`](https://github.com/JeielArguello/tiny_mc/blob/lab2-SIMD/script-comp.sh), [`compare.py`](https://github.com/JeielArguello/tiny_mc/blob/lab2-SIMD/compare.py)
- `lab3`: [`script-comp.sh`](https://github.com/JeielArguello/tiny_mc/blob/lab3/script-comp.sh), [`tam_vs_hilos.sh`](https://github.com/JeielArguello/tiny_mc/blob/lab3/tam_vs_hilos.sh), [`compare.py`](https://github.com/JeielArguello/tiny_mc/blob/lab3/compare.py)
- `lab4`: [`tam_vs_block.sh`](https://github.com/JeielArguello/tiny_mc/blob/lab4/tam_vs_block.sh)
- `codigo-optimizado`: [`script-comp.sh`](https://github.com/JeielArguello/tiny_mc/blob/codigo-optimizado/script-comp.sh)
