#!/bin/bash
for i in {0..3}; do 
    k=32768*$i;
    for j in {0..3}; do 
        echo "L=${k} O=${j}";
        make clean && make CPPFLAGS="-DPHOTONS=${k}" EXTRA_CFLAGS="-O${j}" headless && ./headless; 
    done;
done


echo "Script completed"