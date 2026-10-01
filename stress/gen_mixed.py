"""Genera una escena .rt de prueba con N objetos mezclados (esferas,
cilindros, cuadrados y triangulos a partes iguales) sobre un suelo
reflectante, con 2 luces. Uso: python3 stress/gen_mixed.py 20000 > escena.rt"""
import math
import random
import sys

n = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
random.seed(7)
side = math.sqrt(n) * 4
L = ["R 1920 1080", "A 0.15 200,210,255",
     f"c 0,{side*0.3:.1f},{-side*0.45:.1f} 0,-0.4,1 70",
     f"l {-side/2:.1f},{side:.1f},{-side/2:.1f} 0.7 255,240,220",
     f"l {side/2:.1f},{side*0.6:.1f},{side*0.2:.1f} 0.4 180,200,255",
     "pl 0,0,0 0,1,0 230,230,235 0.2"]


def col():
    return ",".join(str(random.randint(40, 255)) for _ in range(3))


def refl():
    return " 0.5" if random.random() < 0.15 else ""


for i in range(n):
    x = random.uniform(-side / 2, side / 2)
    z = random.uniform(0, side)
    k = i % 4
    s = random.uniform(0.6, 1.8)
    if k == 0:
        L.append(f"sp {x:.2f},{s:.2f},{z:.2f} {2*s:.2f} {col()}{refl()}")
    elif k == 1:
        h = random.uniform(1, 4)
        L.append(f"cy {x:.2f},{h/2:.2f},{z:.2f} 0,1,0 {col()} {s*1.4:.2f} {h:.2f}{refl()}")
    elif k == 2:
        nx, nz = random.uniform(-1, 1), random.uniform(-1, 1)
        L.append(f"sq {x:.2f},{s+0.1:.2f},{z:.2f} {nx:.2f},0.3,{nz:.2f} {2*s:.2f} {col()}{refl()}")
    else:
        y = random.uniform(0, 1)
        L.append(f"tr {x:.2f},{y:.2f},{z:.2f} {x+2*s:.2f},{y:.2f},{z+0.3:.2f} "
                 f"{x+s:.2f},{y+2.5*s:.2f},{z-0.2:.2f} {col()}{refl()}")
print("\n".join(L))
