#!/bin/bash

EXEC=headless
CSV=resultados.csv

echo "photons,block,num_photon_per_threads,seconds,photons_per_sec" > $CSV

for i in {17..26}; do
    PHOTONS=$((2**i))
    blocks=( 128 256 512 )
    for block in "${blocks[@]}"; do
        for j in {1..6}; do
	    num_per_t=$((2**j))
       	    make clean && make EXTRA_CFLAGS="-DNUM_PHOTONS_PER_THREAD=$num_per_t -DPHOTONS=$PHOTONS -DBLOCK_SIZE=$block" headless
            OUTPUT=$(./headless)
            SECONDS_VAL=$(echo "$OUTPUT" | grep -E "# [0-9.]+ seconds" | awk '{print $2}')
            PHOTONS_PER_SEC=$(echo "$OUTPUT" | grep -E "# [0-9.]+ K photons per second" | awk '{print $2}')
            echo "$PHOTONS,$block,$num_per_t,$SECONDS_VAL,$PHOTONS_PER_SEC" >> $CSV
            echo "PHOTONS=$PHOTONS  BLOCK_SIZE=$block NUM_PTHREAD=$num_per_t seconds=$SECONDS_VAL  photons_per_sec=$PHOTONS_PER_SEC"
	done
    done
done
