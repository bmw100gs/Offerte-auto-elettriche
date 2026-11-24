// main.cpp - Pianificatore viaggio Milano -> Napoli in auto elettrica
// Uso: ./viaggio [velocita_kmh] [temperatura_c] [batteria_partenza_%] [tempo|costo]
#include "pianificatore.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdlib>

static std::string ore_min(double ore) {
    int h = (int)ore, m = (int)std::round((ore - h) * 60);
    if (m == 60) { ++h; m = 0; }
    std::ostringstream os;
    os << h << "h" << std::setw(2) << std::setfill('0') << m;
    return os.str();
}

static void stampa_piano(const Veicolo& v, const Piano& p) {
    std::cout << "\n=== " << v.nome() << " ===\n";
    if (!p.fattibile) { std::cout << "  Viaggio impossibile con questi parametri.\n"; return; }
    std::cout << std::left << "  " << std::setw(26) << "Sosta" << std::right
              << std::setw(6) << "km" << std::setw(12) << "batteria"
              << std::setw(8) << "tempo" << std::setw(9) << "costo" << "\n";
    for (const auto& s : p.soste) {
        std::cout << "  " << std::left << std::setw(26) << s.nome << std::right
                  << std::setw(6) << (int)s.km
                  << std::setw(6) << s.arrivo_pct << "->" << std::setw(3) << s.partenza_pct << "%"
                  << std::setw(5) << (int)std::round(s.minuti) << " min"
                  << std::setw(7) << std::fixed << std::setprecision(2) << s.costo << " E\n";
    }
    std::cout << "  Guida " << ore_min(p.ore_guida) << " + ricarica "
              << ore_min(p.ore_ricarica) << " = TOTALE " << ore_min(p.ore_totali())
              << " | " << std::setprecision(2) << p.costo << " E"
              << " | arrivo con " << p.arrivo_pct << "%\n";
}

int main(int argc, char* argv[]) {
    double velocita    = argc > 1 ? std::atof(argv[1]) : 120;
    double temperatura = argc > 2 ? std::atof(argv[2]) : 20;
    int soc_iniziale   = argc > 3 ? std::atoi(argv[3]) : 90;
    Obiettivo obiettivo = (argc > 4 && std::string(argv[4]) == "costo")
                          ? Obiettivo::Costo : Obiettivo::Tempo;

    // Percorso e colonnine di esempio (dati indicativi, non reali)
    std::vector<Tappa> percorso = {
        {"Milano (partenza)",        0,   0, 0.00},
        {"Lodi",                    30, 150, 0.69},
        {"Piacenza",                65,  50, 0.59},
        {"Parma",                  125, 300, 0.79},
        {"Modena",                 175, 150, 0.69},
        {"Bologna",                210, 300, 0.75},
        {"Barberino di Mugello",   280,  50, 0.59},
        {"Firenze Sud",            315, 150, 0.69},
        {"Arezzo",                 380, 300, 0.79},
        {"Chiusi",                 450, 150, 0.69},
        {"Orvieto",                490,  50, 0.55},
        {"Roma Nord",              575, 300, 0.75},
        {"Frosinone",              660, 150, 0.69},
        {"Caserta",                740,  50, 0.59},
        {"Napoli (arrivo)",        770,   0, 0.00},
    };

    std::vector<Veicolo> auto_ = {
        {"Citycar 40 kWh",   40, 14.5,  50},
        {"Media 60 kWh",     60, 15.5, 130},
        {"Berlina 77 kWh",   77, 16.0, 230},
        {"SUV 95 kWh",       95, 20.0, 170},
    };

    std::cout << "Milano -> Napoli (770 km) | " << velocita << " km/h | "
              << temperatura << " C | partenza al " << soc_iniziale << "% | obiettivo: "
              << (obiettivo == Obiettivo::Tempo ? "minimo tempo" : "minimo costo") << "\n";

    for (const auto& v : auto_)
        stampa_piano(v, Pianificatore(v, percorso, velocita, temperatura)
                            .ottimizza(soc_iniziale, obiettivo));

    // Confronto: quale velocita conviene davvero?
    std::cout << "\n=== Tempo totale al variare della velocita ===\n  "
              << std::left << std::setw(18) << "Auto" << std::right;
    std::vector<double> velocita_test = {90, 110, 120, 130};
    for (double vt : velocita_test) std::cout << std::setw(8) << (int)vt;
    std::cout << "  km/h\n";
    for (const auto& v : auto_) {
        std::cout << "  " << std::left << std::setw(18) << v.nome() << std::right;
        for (double vt : velocita_test) {
            Piano p = Pianificatore(v, percorso, vt, temperatura).ottimizza(soc_iniziale, Obiettivo::Tempo);
            std::cout << std::setw(8) << (p.fattibile ? ore_min(p.ore_totali()) : "--");
        }
        std::cout << "\n";
    }
    return 0;
}
