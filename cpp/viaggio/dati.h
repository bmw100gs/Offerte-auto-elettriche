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
    double deviazione_m = 0;  // distanza dall'autostrada (0 = sul percorso)
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
// Generato da strumenti/genera_dati.py: Free To X (Autostrade per l'Italia,
// solo carreggiata verso Milano) e Supercharger Tesla aperti a tutti
// (supercharge.info). Ultimo campo: distanza dall'autostrada in metri.
// Prezzi STIMATI (non pubblicati nei dati).
inline std::vector<Tappa> cattolica_milano() {
    return {
        {"Cattolica (partenza)",                 0.0,   7,   0, 0.00,     0},
        {"Tesla Rimini",                        18.9,  13, 250, 0.69,  1561},
        {"A14 Rubicone Est (Free To X)",        35.7,  15, 300, 0.79,    29},
        {"A14 Bevano Est (Free To X)",          57.5,  16, 200, 0.79,    22},
        {"Tesla Forli",                         65.7,  16, 250, 0.69,  1253},
        {"A14 Sillaro Est (Free To X)",        109.6,  52, 300, 0.79,    27},
        {"A14 La Pioppa Est (Free To X)",      144.7,  51, 300, 0.79,    63},
        {"Tesla Modena",                       164.8,  49, 150, 0.69,   483},
        {"A1 Secchia Est (Free To X)",         179.2,  42, 300, 0.79,    64},
        {"Tesla Reggio Emilia",                197.9,  38, 250, 0.69,  1167},
        {"A1 San Martino Est (Free To X)",     221.5,  41, 300, 0.79,    65},
        {"Tesla Parma",                        224.5,  39, 250, 0.69,  2033},
        {"Tesla Fidenza",                      244.9,  58, 250, 0.69,   729},
        {"A1 Arda Est (Free To X)",            262.4,  62, 300, 0.79,    44},
        {"Tesla Piacenza",                     277.3,  51, 250, 0.69,   484},
        {"A1 Somaglia Est (Free To X)",        292.1,  51, 300, 0.79,    23},
        {"Tesla Lodi",                         312.4,  77, 250, 0.69,   581},
        {"A1 San Zenone Est (Free To X)",      320.5,  84, 300, 0.79,    21},
        {"Milano (arrivo)",                    343.6, 130,   0, 0.00,     0},
    };
}
