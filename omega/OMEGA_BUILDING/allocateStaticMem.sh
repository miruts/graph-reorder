#!/bin/bash

#This script creates some static addresses that are used  by the core to 
#configure the accelerators with various parameters

#How to run:
# ./allocateStticMem.sh

if [ ! -f "omega.tmp" ]; then 
    touch "omega.tmp"
    for i in `seq 1 10000`;
    do
        echo "0123456789" >> "omega.tmp"
    done
fi

ld -b binary -r -o /tmp/omega_static.tmp omega.tmp
objcopy --rename-section .data=.testmem /tmp/omega_static.tmp

rm omega.tmp
