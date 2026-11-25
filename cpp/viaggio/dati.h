// dati.h - Auto e percorso
#pragma once
#include "veicolo.h"
#include <string>
#include <vector>

struct Tappa {
    std::string nome;
    double km;          // progressiva dalla partenza
    double quota_m;     // altitudine
    double kw;          // potenza colonnina (0 = nessuna)
    double prezzo_kwh;  // euro
};

// Skoda Elroq 85 - motore posteriore APP550
// Batteria, potenza DC e tempi da scheda Skoda / ev-database / evkx.
// Massa, Cx e area frontale sono STIME; la curva DC e' calibrata sui tempi
// reali misurati (circa 34 minuti 10-80%, non i 28 dichiarati).
inline SchedaTecnica skoda_elroq_85() {
    return {
        "Skoda Elroq 85", "APP550 (posteriore)", 210,
        82, 77,
        2100,
        0.26, 2.55,
        0.0085,
        0.88, 0.65, 0.95,
        175,
        {{0, 100}, {10, 135}, {20, 150}, {30, 140}, {40, 120}, {50, 100},
         {60, 85}, {70, 72}, {80, 55}, {90, 32}, {100, 10}},
    };
}

// Cattolica -> Milano (A14 + A1), 343.6 km sul percorso reale OSRM.
// Colonnine Free To X ufficiali (Autostrade per l'Italia), solo carreggiata
// verso Milano. Quote da EU-DEM. Prezzi STIMATI (non pubblicati).
inline std::vector<Tappa> cattolica_milano() {
    return {
        {"Cattolica (partenza)",        0,  7,   0, 0.00},
        {"A14 Rubicone Est",            35.7,  15, 300, 0.79},
        {"A14 Bevano Est",              57.5,  16, 200, 0.79},
        {"A14 Sillaro Est",            109.6,  52, 300, 0.79},
        {"A14 La Pioppa Est",          144.7,  51, 300, 0.79},
        {"A1 Secchia Est",             179.2,  42, 300, 0.79},
        {"A1 San Martino Est",         221.5,  41, 300, 0.79},
        {"A1 Arda Est",                262.4,  62, 300, 0.79},
        {"A1 Somaglia Est",            292.1,  51, 300, 0.79},
        {"A1 San Zenone Est",          320.5,  84, 300, 0.79},
        {"Milano (arrivo)",            343.6, 130,   0, 0.00},
    };
}
