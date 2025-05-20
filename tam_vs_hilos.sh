#!/bin/bash

EXEC=headless
CSV=resultados.csv

echo "photons,threads,seconds,photons_per_sec" > $CSV

for i in {20..28}; do
    PHOTONS=$((2**i))
    for THREADS in {1..6}; do
        export OMP_NUM_THREADS=$THREADS
        make clean && make CC="icx" CPPFLAGS="-DPHOTONS=$PHOTONS -qopenmp" EXTRA_CFLAGS="-O3" TINY_LDFLAGS="-lm -qopenmp" headless
        OUTPUT=$(./headless)
        SECONDS_VAL=$(echo "$OUTPUT" | grep -E "# [0-9.]+ seconds" | awk '{print $2}')
        PHOTONS_PER_SEC=$(echo "$OUTPUT" | grep -E "# [0-9.]+ K photons per second" | awk '{print $2}')
        echo "$PHOTONS,$THREADS,$SECONDS_VAL,$PHOTONS_PER_SEC" >> $CSV
        echo "PHOTONS=$PHOTONS  OMP_NUM_THREADS=$THREADS  seconds=$SECONDS_VAL  photons_per_sec=$PHOTONS_PER_SEC"
    done
done