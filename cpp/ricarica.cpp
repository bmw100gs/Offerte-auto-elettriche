// Calcolatore ricarica auto elettrica
// Compila:  g++ -o ricarica ricarica.cpp
// Esegui:   ./ricarica
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

struct Colonnina {
    std::string nome;
    double potenza_kw;   // potenza di ricarica
    double prezzo_kwh;   // euro per kWh
};

int main() {
    double batteria_kwh, carica_attuale, carica_obiettivo, consumo_kwh_100km;

    std::cout << "Capacita batteria (kWh): ";        std::cin >> batteria_kwh;
    std::cout << "Carica attuale (%): ";             std::cin >> carica_attuale;
    std::cout << "Carica desiderata (%): ";          std::cin >> carica_obiettivo;
    std::cout << "Consumo medio (kWh/100 km): ";     std::cin >> consumo_kwh_100km;

    double energia = batteria_kwh * (carica_obiettivo - carica_attuale) / 100.0;
    double km_guadagnati = energia / consumo_kwh_100km * 100.0;
    double autonomia_finale = batteria_kwh * carica_obiettivo / 100.0
                              / consumo_kwh_100km * 100.0;

    std::vector<Colonnina> colonnine = {
        {"Presa di casa",         2.3, 0.25},
        {"Wallbox di casa",       7.4, 0.25},
        {"Colonnina pubblica AC", 22.0, 0.59},
        {"Fast charge DC",        50.0, 0.69},
        {"Ultra fast DC",        150.0, 0.79},
    };

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\nEnergia da caricare: " << energia << " kWh\n";
    std::cout << "Km guadagnati:       " << km_guadagnati << " km\n";
    std::cout << "Autonomia finale:    " << autonomia_finale << " km\n\n";

    std::cout << std::left << std::setw(24) << "Tipo ricarica"
              << std::right << std::setw(10) << "Tempo"
              << std::setw(10) << "Costo" << std::setw(14) << "Euro/100km" << "\n";
    std::cout << std::string(58, '-') << "\n";

    for (const auto& c : colonnine) {
        double ore = energia / (c.potenza_kw * 0.9);   // 90% efficienza
        int h = static_cast<int>(ore);
        int m = static_cast<int>((ore - h) * 60);
        double costo = energia / 0.9 * c.prezzo_kwh;
        double costo_100 = consumo_kwh_100km / 0.9 * c.prezzo_kwh;

        std::cout << std::left << std::setw(24) << c.nome << std::right
                  << std::setw(6) << h << "h" << std::setw(2) << std::setfill('0') << m
                  << std::setfill(' ') << "m"
                  << std::setw(8) << std::setprecision(2) << costo << " E"
                  << std::setw(11) << costo_100 << " E\n" << std::setprecision(1);
    }
    return 0;
}
