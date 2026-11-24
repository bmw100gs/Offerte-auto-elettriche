// pianificatore.h - Ottimizzazione delle soste di ricarica
// Algoritmo: programmazione dinamica sugli stati (tappa, % batteria).
#pragma once
#include "veicolo.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>

struct Tappa {
    std::string nome;
    double km;           // distanza dalla partenza
    double potenza_kw;   // 0 = nessuna colonnina
    double prezzo_kwh;
};

struct Sosta {
    std::string nome;
    double km;
    int arrivo_pct, partenza_pct;
    double minuti, costo;
};

struct Piano {
    bool fattibile = false;
    std::vector<Sosta> soste;
    double ore_guida = 0, ore_ricarica = 0, costo = 0;
    int arrivo_pct = 0;
    double ore_totali() const { return ore_guida + ore_ricarica; }
};

enum class Obiettivo { Tempo, Costo };

class Pianificatore {
public:
    static constexpr int RISERVA = 10;           // % minima all'arrivo
    static constexpr double EFFICIENZA = 0.92;   // perdite in ricarica
    static constexpr double MINUTI_SOSTA = 5.0;  // parcheggio, cavo, pagamento
    static constexpr double EURO_ORA = 15.0;     // valore del tempo (obiettivo costo)

    Pianificatore(const Veicolo& v, std::vector<Tappa> tappe,
                  double velocita, double temperatura)
        : v_(v), tappe_(std::move(tappe)), vel_(velocita), temp_(temperatura) {}

    Piano ottimizza(int soc_iniziale, Obiettivo obiettivo) const {
        const int N = tappe_.size();
        const double INF = std::numeric_limits<double>::infinity();
        // migliore[i][s] = punteggio minimo arrivando alla tappa i con s%
        std::vector<std::vector<double>> migliore(N, std::vector<double>(101, INF));
        struct Da { int tappa = -1, soc_arrivo = -1, soc_partenza = -1; };
        std::vector<std::vector<Da>> da(N, std::vector<Da>(101));

        migliore[0][soc_iniziale] = 0;
        const double consumo = v_.consumo(vel_, temp_);

        for (int i = 0; i < N - 1; ++i) {
            auto minuti_cumulati = curva_tempi(tappe_[i].potenza_kw);
            for (int s = 0; s <= 100; ++s) {
                if (migliore[i][s] == INF) continue;
                // A ogni tappa si sceglie fino a che % caricare...
                int t_max = tappe_[i].potenza_kw > 0 ? 100 : s;
                for (int t = s; t <= t_max; ++t) {
                    double min_ric = minuti_cumulati[t] - minuti_cumulati[s];
                    if (t > s) min_ric += MINUTI_SOSTA;
                    double euro = energia(t - s) / EFFICIENZA * tappe_[i].prezzo_kwh;
                    double costo_ric = punteggio(min_ric / 60.0, euro, obiettivo);
                    // ...e fino a quale tappa successiva arrivare
                    for (int j = i + 1; j < N; ++j) {
                        double dist = tappe_[j].km - tappe_[i].km;
                        int usati = (int)std::ceil(consumo * dist / 100.0
                                                   / v_.batteria() * 100.0);
                        int arrivo = t - usati;
                        if (arrivo < RISERVA) break;
                        double val = migliore[i][s] + costo_ric
                                   + punteggio(dist / vel_, 0, obiettivo);
                        if (val < migliore[j][arrivo]) {
                            migliore[j][arrivo] = val;
                            da[j][arrivo] = {i, s, t};
                        }
                    }
                }
            }
        }

        // Miglior stato all'arrivo
        Piano p;
        int best = -1;
        for (int s = 0; s <= 100; ++s)
            if (migliore[N - 1][s] < INF &&
                (best < 0 || migliore[N - 1][s] < migliore[N - 1][best])) best = s;
        if (best < 0) return p;

        // Ricostruzione all'indietro del percorso ottimo
        p.fattibile = true;
        p.arrivo_pct = best;
        int j = N - 1, s = best;
        while (j > 0) {
            Da d = da[j][s];
            const Tappa& t = tappe_[d.tappa];
            p.ore_guida += (tappe_[j].km - t.km) / vel_;
            if (d.soc_partenza > d.soc_arrivo) {
                auto cum = curva_tempi(t.potenza_kw);
                double min = cum[d.soc_partenza] - cum[d.soc_arrivo] + MINUTI_SOSTA;
                double euro = energia(d.soc_partenza - d.soc_arrivo)
                              / EFFICIENZA * t.prezzo_kwh;
                p.soste.insert(p.soste.begin(),
                    {t.nome, t.km, d.soc_arrivo, d.soc_partenza, min, euro});
                p.ore_ricarica += min / 60.0;
                p.costo += euro;
            }
            j = d.tappa; s = d.soc_arrivo;
        }
        return p;
    }

private:
    double energia(int punti_pct) const { return v_.batteria() * punti_pct / 100.0; }

    double punteggio(double ore, double euro, Obiettivo o) const {
        return o == Obiettivo::Tempo ? ore : euro + ore * EURO_ORA;
    }

    // minuti[s] = minuti per caricare da 0% a s% (integrazione a passi dell'1%)
    std::vector<double> curva_tempi(double potenza_colonnina) const {
        std::vector<double> minuti(101, 0.0);
        if (potenza_colonnina <= 0) return minuti;
        for (int s = 1; s <= 100; ++s) {
            double kw = v_.potenza_ricarica(s - 0.5, potenza_colonnina);
            minuti[s] = minuti[s - 1] + energia(1) / EFFICIENZA / kw * 60.0;
        }
        return minuti;
    }

    const Veicolo& v_;
    std::vector<Tappa> tappe_;
    double vel_, temp_;
};
