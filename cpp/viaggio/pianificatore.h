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
public:
    struct CostoSosta { double minuti, kwh_rete, euro; };

    // Sosta alla tappa i caricando da a% a b%. Se la colonnina e' fuori
    // dall'autostrada si aggiungono andata e ritorno (percorso stradale stimato
    // 1,3 volte la linea d'aria, a 50 km/h) e l'energia per farli, che va
    // ricaricata in piu'.
    CostoSosta costo_sosta(int i, double a, double b) const {
        const Tappa& t = t_[i];
        double km_dev = t.deviazione_m > RAGGIO_AUTOSTRADA_M ? 2 * 1.3 * t.deviazione_m / 1000 : 0;
        Condizioni urbano = p_.cond; urbano.velocita_kmh = 50;
        double kwh_dev = km_dev > 0 ? v_.energia_tratto(km_dev, 0, urbano) : 0;
        double min = v_.minuti_ricarica(a, b, t.kw) + p_.minuti_sosta + km_dev / 50 * 60
                   + kwh_dev / v_.potenza_ricarica(b, t.kw) * 60;
        double rete = (v_.batteria() * (b - a) / 100 + kwh_dev) / v_.scheda().eff_ricarica_dc;
        return {min, rete, rete * t.prezzo_kwh};
    }
private:
    static constexpr double RAGGIO_AUTOSTRADA_M = 400;
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
                    double min = 0, euro = 0;
                    if (t > s) {
                        CostoSosta extra = costo_sosta(i, (double)t / R, (double)t / R);
                        min = cum[t] - cum[s] + extra.minuti;
                        euro = (v_.batteria() * (t - s) / S / v_.scheda().eff_ricarica_dc
                                + extra.kwh_rete) * t_[i].prezzo_kwh;
                    }
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
                CostoSosta c = costo_sosta(d.i, a, b);
                p.soste.insert(p.soste.begin(), {d.i, a, b, c.minuti, c.kwh_rete, c.euro});
                p.ore_ricarica += c.minuti / 60;
                p.costo += c.euro;
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
