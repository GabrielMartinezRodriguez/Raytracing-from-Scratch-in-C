#!/bin/bash
# Uso: stress/climb.sh <nombre> <aa> [variables de entorno extra...]
# Recorre San Miguel 4K (120 imagenes con camara en movimiento) y compara
# las 3 imagenes de control con la referencia. Imprime ms y PSNR medio.
name=$1; aa=$2; shift 2
out=~/Desktop/code/rt-models/ref/$name
line=$(env "$@" ./rt_gpu stress/sanmiguel_4k.rt --obj ~/Desktop/code/rt-models/San_Miguel/san-miguel.obj --path 120 $out $aa 2>&1 | grep '^path')
ms=$(echo "$line" | sed 's/.*mediana \([0-9.]*\) ms.*/\1/')
p95=$(echo "$line" | sed 's/.*p95 \([0-9.]*\) ms.*/\1/')
tot=0
for f in 040 080 119; do
  p=$(magick compare -metric PSNR ~/Desktop/code/rt-models/ref/verdad_f$f.png ${out}_f$f.png null: 2>&1 | cut -d' ' -f1)
  tot=$(echo "$tot + $p" | bc -l)
done
printf "%s: %s ms (p95 %s) | PSNR %.2f dB\n" "$name" "$ms" "$p95" "$(echo "$tot / 3" | bc -l)"
