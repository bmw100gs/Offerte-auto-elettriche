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

// Cattolica -> Milano (A14 + A1), circa 335 km.
// Aree di servizio reali, ma progressive, potenze e prezzi INDICATIVI.
inline std::vector<Tappa> cattolica_milano() {
    return {
        {"Cattolica (partenza)",        0,  10,   0, 0.00},
        {"A14 Area Rubicone",          38,  30, 150, 0.69},
        {"A14 Area Santerno (Imola)",  90,  45, 300, 0.79},
        {"A14 Area La Pioppa",        110,  50, 150, 0.69},
        {"A1 Area Secchia (Modena)",  163,  35, 300, 0.75},
        {"A1 Area Taro (Parma)",      212,  60, 150, 0.69},
        {"A1 Area Arda (Fiorenzuola)",250,  80,  50, 0.59},
        {"A1 Area Somaglia (Lodi)",   292,  60, 300, 0.79},
        {"A1 Area San Zenone",        312,  85, 150, 0.69},
        {"Milano (arrivo)",           335, 120,   0, 0.00},
    };
}
