// veicolo.h - Modello fisico semplificato di un'auto elettrica
#pragma once
#include <algorithm>
#include <string>

class Veicolo {
public:
    Veicolo(std::string nome, double batteria_kwh, double consumo_base,
            double potenza_max_kw)
        : nome_(std::move(nome)), batteria_(batteria_kwh),
          consumo_base_(consumo_base), potenza_max_(potenza_max_kw) {}

    const std::string& nome() const { return nome_; }
    double batteria() const { return batteria_; }

    // Consumo in kWh/100km: cresce col quadrato della velocita (resistenza
    // aerodinamica) e con freddo (riscaldamento) o caldo (climatizzatore).
    double consumo(double velocita_kmh, double temperatura_c) const {
        double rapporto = velocita_kmh / 100.0;
        double c = consumo_base_ * (0.45 + 0.55 * rapporto * rapporto);
        if (temperatura_c < 15) c *= 1.0 + 0.012 * (15 - temperatura_c);
        if (temperatura_c > 25) c *= 1.0 + 0.006 * (temperatura_c - 25);
        return c;
    }

    // Curva di ricarica: piena potenza fino al 50%, poi cala fino all'80%,
    // poi crolla. Limitata anche dalla potenza della colonnina.
    double potenza_ricarica(double soc_pct, double potenza_colonnina) const {
        double fattore;
        if (soc_pct < 50)      fattore = 1.0;
        else if (soc_pct < 80) fattore = 1.0 - 0.5 * (soc_pct - 50) / 30.0;
        else                   fattore = 0.5 - 0.35 * (soc_pct - 80) / 20.0;
        return std::min(potenza_max_ * fattore, potenza_colonnina);
    }

private:
    std::string nome_;
    double batteria_;      // kWh utili
    double consumo_base_;  // kWh/100km a 100 km/h, 20 gradi
    double potenza_max_;   // kW massimi in DC
};
