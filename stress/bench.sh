#!/bin/bash
# Mejor de 5 ejecuciones (cada una ya es la mediana de 15 frames)
for i in 1 2 3 4 5; do ./rt_gpu ${1:-stress/mixed_20k.rt} --bench ${2:-1} | grep bench | sed 's/.*: \([0-9.]*\) ms/\1/'; done | sort -n | head -1
