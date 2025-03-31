#!/bin/bash

# Crear o vaciar el archivo CSV
echo "PHOTONS,Compiler,Flags,Task-clock,Context-switches,CPU-migrations,Page-faults,Cycles,Instructions,Branches,Branch-misses" > stats.csv

source /opt/intel/oneapi/setvars.sh
flags_array=("" "-march=native" "-funroll-loops" "-march=native -funroll-loops")
compiler_array=("gcc" "clang" "icx")
for compiler in "${compiler_array[@]}"; do
    for flags in "${flags_array[@]}"; do
        for i in {17..21}; do 
            k=$((2**i));
            for j in {0..3}; do 
                echo "PHOTONS=${k} Compiler=${compiler} Flags=${flags}";
                make clean && make CC="${compiler}" CPPFLAGS="-DPHOTONS=${k}" EXTRA_CFLAGS="-O${j} ${flags}" headless
                
                # Ejecutar perf stat y capturar la salida
                perf_output=$(perf stat -x, -e task-clock,context-switches,cpu-migrations,page-faults,cycles,instructions,branches,branch-misses ./headless 2>&1)

                # Extraer estadísticas de la salida de perf
                task_clock=$(echo "$perf_output" | grep ',task-clock,' | awk -F, '{print $1}')
                context_switches=$(echo "$perf_output" | grep ',context-switches,' | awk -F, '{print $1}')
                cpu_migrations=$(echo "$perf_output" | grep ',cpu-migrations,' | awk -F, '{print $1}')
                page_faults=$(echo "$perf_output" | grep ',page-faults,' | awk -F, '{print $1}')
                cycles=$(echo "$perf_output" | grep ',cycles,' | awk -F, '{print $1}')
                instructions=$(echo "$perf_output" | grep ',instructions,' | awk -F, '{print $1}')
                branches=$(echo "$perf_output" | grep ',branches,' | awk -F, '{print $1}')
                branch_misses=$(echo "$perf_output" | grep ',branch-misses,' | awk -F, '{print $1}')

                # Agregar las estadísticas al archivo CSV
                echo "${k},${compiler},-O${j} ${flags},${task_clock},${context_switches},${cpu_migrations},${page_faults},${cycles},${instructions},${branches},${branch_misses}" >> stats.csv
            done;
        done
    done
done