// pianificatore.h - Strategia di ricarica ottimale
// Programmazione dinamica sugli stati (tappa, % batteria) con passo 0.5%.
#pragma once
#include "dati.h"
#include <cmath>
#include <limits>
#include <vector>

struct Passo {                 // una tappa in cui ci si ferma a caricare
    int tappa;
    double soc_arrivo, soc_partenza, minuti, kwh_rete, costo;
};

struct Piano {
    bool fattibile = false;
    std::vector<Passo> soste;
    double ore_guida = 0, ore_ricarica = 0, costo = 0, kwh_consumati = 0;
    double soc_arrivo = 0;
    double ore_totali() const { return ore_guida + ore_ricarica; }
};

enum class Obiettivo { Tempo, Costo };

struct Parametri {
    Condizioni cond;
    double soc_partenza = 90;
    double riserva = 10;           // % minima a ogni arrivo
    double soc_arrivo_min = 10;    // % minima a destinazione
    Obiettivo obiettivo = Obiettivo::Tempo;
    double minuti_sosta = 5;       // parcheggio, cavo, pagamento
    double euro_ora = 15;          // valore del tempo (modalita costo)
};

class Pianificatore {
    static constexpr int R = 2;            // passi per punto percentuale
    static constexpr int S = 100 * R;      // numero stati batteria
public:
    Pianificatore(const Veicolo& v, const std::vector<Tappa>& t, const Parametri& p)
        : v_(v), t_(t), p_(p) {}

    Piano ottimizza() const {
        const int N = t_.size();
        const double INF = std::numeric_limits<double>::infinity();

        // Energia tra ogni coppia di tappe (somma dei tratti intermedi)
        std::vector<std::vector<double>> kwh(N, std::vector<double>(N, 0));
        for (int i = 0; i < N; ++i)
            for (int j = i + 1; j < N; ++j)
                kwh[i][j] = kwh[i][j - 1] + v_.energia_tratto(
                    t_[j].km - t_[j - 1].km, t_[j].quota_m - t_[j - 1].quota_m, p_.cond);

        std::vector<std::vector<double>> best(N, std::vector<double>(S + 1, INF));
        struct Da { int i = -1, s = -1, t = -1; };
        std::vector<std::vector<Da>> da(N, std::vector<Da>(S + 1));

        int s0 = (int)std::round(p_.soc_partenza * R);
        best[0][s0] = 0;
        int min_tappa = (int)std::ceil(p_.riserva * R);
        int min_fine = (int)std::ceil(std::max(p_.riserva, p_.soc_arrivo_min) * R);

        for (int i = 0; i < N - 1; ++i) {
            std::vector<double> cum(S + 1, 0);          // minuti da 0 a k
            if (t_[i].kw > 0)
                for (int k = 1; k <= S; ++k)
                    cum[k] = cum[k - 1] + v_.minuti_ricarica((k - 1.0) / R, (double)k / R, t_[i].kw);

            for (int s = 0; s <= S; ++s) {
                if (best[i][s] == INF) continue;
                for (int t = s; t <= (t_[i].kw > 0 ? S : s); ++t) {
                    double min = cum[t] - cum[s] + (t > s ? p_.minuti_sosta : 0);
                    double euro = v_.batteria() * (t - s) / S / v_.scheda().eff_ricarica_dc
                                  * t_[i].prezzo_kwh;
                    double c_ric = punteggio(min / 60, euro);
                    for (int j = i + 1; j < N; ++j) {
                        int usati = (int)std::ceil(kwh[i][j] / v_.batteria() * S - 1e-9);
                        int arrivo = t - usati;
                        if (arrivo < (j == N - 1 ? min_fine : min_tappa)) {
                            if (arrivo < min_tappa) break;
                            continue;
                        }
                        double ore = (t_[j].km - t_[i].km) / p_.cond.velocita_kmh;
                        double val = best[i][s] + c_ric + punteggio(ore, 0);
                        if (val < best[j][arrivo] - 1e-12) {
                            best[j][arrivo] = val;
                            da[j][arrivo] = {i, s, t};
                        }
                    }
                }
            }
        }

        Piano p;
        int fine = -1;
        for (int s = 0; s <= S; ++s)
            if (best[N - 1][s] < INF && (fine < 0 || best[N - 1][s] < best[N - 1][fine])) fine = s;
        if (fine < 0) return p;

        p.fattibile = true;
        p.soc_arrivo = (double)fine / R;
        p.kwh_consumati = kwh[0][N - 1];
        for (int j = N - 1, s = fine; j > 0;) {
            Da d = da[j][s];
            p.ore_guida += (t_[j].km - t_[d.i].km) / p_.cond.velocita_kmh;
            if (d.t > d.s) {
                double a = (double)d.s / R, b = (double)d.t / R;
                double min = v_.minuti_ricarica(a, b, t_[d.i].kw) + p_.minuti_sosta;
                double rete = v_.batteria() * (b - a) / 100 / v_.scheda().eff_ricarica_dc;
                p.soste.insert(p.soste.begin(), {d.i, a, b, min, rete, rete * t_[d.i].prezzo_kwh});
                p.ore_ricarica += min / 60;
                p.costo += rete * t_[d.i].prezzo_kwh;
            }
            j = d.i; s = d.s;
        }
        return p;
    }

private:
    double punteggio(double ore, double euro) const {
        return p_.obiettivo == Obiettivo::Tempo ? ore : euro + ore * p_.euro_ora;
    }
    const Veicolo& v_;
    const std::vector<Tappa>& t_;
    Parametri p_;
};
