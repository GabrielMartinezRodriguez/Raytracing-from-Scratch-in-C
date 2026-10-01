"""Media del error FLIP (0 = identica, mas bajo es mejor) de las 3 imagenes
de control frente a la referencia. Uso: flip_mean.py <ref_prefijo> <prefijo>"""
import sys
import flip_evaluator as flip

ref, test = sys.argv[1], sys.argv[2]
errs = []
for f in ("040", "080", "119"):
    _, mean, _ = flip.evaluate(f"{ref}_f{f}.png", f"{test}_f{f}.png", "LDR")
    errs.append(mean)
print(f"{sum(errs) / len(errs):.5f}")
