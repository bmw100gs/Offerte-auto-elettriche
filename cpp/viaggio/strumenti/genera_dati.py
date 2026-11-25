#!/usr/bin/env python3
"""Rigenera l'elenco tappe in dati.h dal CSV prodotto da scarica_percorso.py.
Uso: python3 strumenti/genera_dati.py percorsi/cattolica_milano.csv dati.h"""
import re
import sys

csv, header = sys.argv[1], sys.argv[2]
righe = [l.rstrip("\n").split(",") for l in open(csv)]
prof = [(float(r[1]), float(r[2])) for r in righe if r[0] == "P"]
km_fine = prof[-1][0]


def quota(km):
    for (k0, q0), (k1, q1) in zip(prof, prof[1:]):
        if km <= k1:
            return q0 + (km - k0) / (k1 - k0) * (q1 - q0)
    return prof[-1][1]


def riga(nome, km, kw, prezzo, dev):
    return (f'        {{"{nome}",{" " * max(1, 34 - len(nome))}{km:6.1f}, {round(quota(km)):3d}, '
            f'{kw:>3}, {prezzo}, {dev:>5}}},\n')


tappe = riga("Cattolica (partenza)", 0, 0, "0.00", 0)
for r in righe:
    if r[0] == "C" and 1 < float(r[1]) < km_fine - 1:
        tappe += riga(r[2], float(r[1]), r[3], r[4], r[5])
tappe += riga("Milano (arrivo)", km_fine, 0, "0.00", 0)

s = open(header).read()
s = re.sub(r"// Cattolica -> Milano.*?\n}\n",
           f"// Cattolica -> Milano (A14 + A1), {km_fine:.1f} km sul percorso reale OSRM.\n"
           "// Generato da strumenti/genera_dati.py: Free To X (Autostrade per l'Italia,\n"
           "// solo carreggiata verso Milano) e Supercharger Tesla aperti a tutti\n"
           "// (supercharge.info). Ultimo campo: distanza dall'autostrada in metri.\n"
           "// Prezzi STIMATI (non pubblicati nei dati).\n"
           "inline std::vector<Tappa> cattolica_milano() {\n    return {\n" + tappe + "    };\n}\n",
           s, flags=re.S)
open(header, "w").write(s)
print("Aggiornato", header)
