// veicolo.h - Modello fisico di un'auto elettrica
// Il consumo e' calcolato dalle forze reali: aerodinamica, rotolamento,
// pendenza, rendimento del motore e potenza assorbita dal clima.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

struct Condizioni {
    double velocita_kmh = 120;
    double temperatura_c = 15;
    int passeggeri = 1;        // conducente incluso
    double bagagli_kg = 20;
};

struct SchedaTecnica {
    std::string nome, motore;
    double potenza_motore_kw;
    double batteria_lorda_kwh, batteria_netta_kwh;
    double massa_kg;            // a vuoto
    double cx, area_frontale_m2;
    double crr;                 // coefficiente di rotolamento pneumatici
    double eff_trazione;        // batteria -> ruote
    double eff_rigenerazione;   // recupero in discesa
    double eff_ricarica_dc;     // colonnina -> batteria
    double potenza_dc_max_kw;
    std::vector<std::pair<double, double>> curva_dc;  // (% batteria, kW)
};

class Veicolo {
public:
    explicit Veicolo(SchedaTecnica s) : s_(std::move(s)) {}
    const SchedaTecnica& scheda() const { return s_; }
    double batteria() const { return s_.batteria_netta_kwh; }

    double massa(const Condizioni& c) const {
        return s_.massa_kg + 75.0 * c.passeggeri + c.bagagli_kg;
    }

    // Elettronica di bordo + climatizzazione con pompa di calore (kW)
    static double potenza_servizi(double temp) {
        double p = 0.35;
        if (temp < 18) p += 0.11 * (18 - temp);   // riscaldamento
        if (temp > 24) p += 0.09 * (temp - 24);   // aria condizionata
        return p;
    }

    // Energia prelevata dalla batteria (kWh) per un tratto di strada
    double energia_tratto(double km, double dislivello_m, const Condizioni& c) const {
        const double g = 9.81;
        double v = c.velocita_kmh / 3.6;                                   // m/s
        double rho = 101325.0 / (287.05 * (273.15 + c.temperatura_c));     // densita aria
        double m = massa(c);

        double f_aero = 0.5 * rho * s_.cx * s_.area_frontale_m2 * v * v;  // N
        double f_rotolamento = s_.crr * m * g;                             // N
        double j_ruote = (f_aero + f_rotolamento) * km * 1000.0;
        double j_pendenza = m * g * dislivello_m;

        double j_batteria = j_ruote / s_.eff_trazione;
        j_batteria += j_pendenza > 0 ? j_pendenza / s_.eff_trazione
                                     : j_pendenza * s_.eff_rigenerazione;

        double kwh = j_batteria / 3.6e6
                   + potenza_servizi(c.temperatura_c) * km / c.velocita_kmh;
        if (c.temperatura_c < 10)             // batteria fredda: piu' perdite
            kwh *= 1.0 + 0.004 * (10 - c.temperatura_c);
        return kwh;
    }

    double consumo_100km(const Condizioni& c) const { return energia_tratto(100, 0, c); }

    // Potenza di ricarica DC alla percentuale data (interpolazione sulla curva)
    double potenza_ricarica(double soc, double kw_colonnina) const {
        const auto& cv = s_.curva_dc;
        double kw = cv.back().second;
        for (size_t i = 1; i < cv.size(); ++i) {
            if (soc <= cv[i].first) {
                double f = (soc - cv[i - 1].first) / (cv[i].first - cv[i - 1].first);
                kw = cv[i - 1].second + f * (cv[i].second - cv[i - 1].second);
                break;
            }
        }
        return std::min(kw, kw_colonnina);
    }

    // Minuti per caricare da a% a b% su una colonnina di data potenza
    double minuti_ricarica(double a, double b, double kw_colonnina) const {
        double min = 0;
        for (double s = a; s < b - 1e-9; s += 0.5) {
            double passo = std::min(0.5, b - s);
            double kwh = batteria() * passo / 100.0;
            min += kwh / potenza_ricarica(s + passo / 2, kw_colonnina) * 60.0;
        }
        return min;
    }

private:
    SchedaTecnica s_;
};
