#!/bin/bash
# Uso: stress/climb_gi.sh <nombre> [variables de entorno...]
# Recorrido de 120 imagenes de San Miguel en 4K con luz global; imprime la
# mediana de ms de GPU y el FLIP medio de las 3 imagenes de control.
name=$1; shift
G=~/Desktop/code/rt-models/gi
line=$(env RT_GI=1 "$@" ${BIN:-./rt_gpu} stress/sanmiguel_4k.rt --obj ~/Desktop/code/rt-models/San_Miguel/san-miguel.obj --path 120 $G/$name 1 2>&1 | grep '^path')
ms=$(echo "$line" | sed 's/.*mediana \([0-9.]*\) ms.*/\1/')
p95=$(echo "$line" | sed 's/.*p95 \([0-9.]*\) ms.*/\1/')
fl=$(/private/tmp/claude-501/-Users-gabrielmartinezrodriguez-orca-workspaces-test-sandlance/8e60f652-7d3c-4502-b431-0247220f9424/scratchpad/flipenv/bin/python stress/flip_mean.py $G/ref $G/$name)
echo "$name: $ms ms (p95 $p95) | FLIP $fl"
