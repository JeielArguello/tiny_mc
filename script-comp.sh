#!/bin/bash

# Crear o vaciar el archivo CSV
echo "PHOTONS,O,Compiler,Flags,Task-clock,Context-switches,CPU-migrations,Page-faults,Cycles,Instructions,Branches,Branch-misses" > stats.csv

source /opt/intel/oneapi/setvars.sh
flags_array=("" "-march=native" "-funroll-all-loops" "-march=native -funroll-loops")
compiler_array=("gcc" "clang" "icx")
for compiler in "${compiler_array[@]}"; do
    for flags in "${flags_array[@]}"; do
        for i in {17..21}; do 
            k=$((2**i));
            for j in {0..3}; do 
                echo "PHOTONS=${k} O=${j}" compiling with ${compiler} ${flags};
                make clean && make CC="${compiler}" CPPFLAGS="-DPHOTONS=${k}" EXTRA_CFLAGS="-O${j} ${flags}" headless
                perf stat -o temp.txt ./headless
                # Extraer estadísticas y agregar al archivo CSV
                awk -v photons=${k} -v opt=${j} -v compiler=${compiler} -v flags="-O${j} ${flags}" '
                /task-clock/ {task_clock=$1}
                /context-switches/ {context_switches=$1}
                /cpu-migrations/ {cpu_migrations=$1}
                /page-faults/ {page_faults=$1}
                /cycles/ {cycles=$1}
                /instructions/ {instructions=$1}
                /branches/ {branches=$1}
                /branch-misses/ {branch_misses=$1}
                END {print photons "," opt "," compiler "," flags "," task_clock "," context_switches "," cpu_migrations "," page_faults "," cycles "," instructions "," branches "," branch_misses}' temp.txt >> stats.csv
            done;
        done
    done
done

# Eliminar archivo temporal
rm temp.txt

echo "Script completed"