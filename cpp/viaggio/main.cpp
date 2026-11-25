// main.cpp - Electric Nav: pianificatore di viaggio per auto elettrica
// Uso: ./viaggio [velocita=120] [temp=15] [partenza=90] [arrivo=10]
//                [riserva=10] [passeggeri=1] [ora=08:30] [obiettivo=tempo|costo]
#include "pianificatore.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

static std::string durata(double ore) {
    int min = (int)std::round(ore * 60);
    std::ostringstream os;
    os << min / 60 << "h" << std::setw(2) << std::setfill('0') << min % 60;
    return os.str();
}

static std::string orario(double minuti) {
    int m = ((int)std::round(minuti)) % (24 * 60);
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << m / 60 << ":"
       << std::setw(2) << std::setfill('0') << m % 60;
    return os.str();
}

int main(int argc, char* argv[]) {
    std::map<std::string, std::string> arg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto eq = a.find('=');
        if (eq != std::string::npos) arg[a.substr(0, eq)] = a.substr(eq + 1);
    }
    auto num = [&](const std::string& k, double def) {
        return arg.count(k) ? std::atof(arg[k].c_str()) : def;
    };

    Parametri par;
    par.cond.velocita_kmh  = num("velocita", 120);
    par.cond.temperatura_c = num("temp", 15);
    par.cond.passeggeri    = (int)num("passeggeri", 1);
    par.soc_partenza       = num("partenza", 90);
    par.riserva            = num("riserva", 10);
    par.soc_arrivo_min     = num("arrivo", 10);
    if (arg["obiettivo"] == "costo") par.obiettivo = Obiettivo::Costo;
    std::string ora = arg.count("ora") ? arg["ora"] : "08:30";
    double t0 = std::atoi(ora.substr(0, ora.find(':')).c_str()) * 60
              + std::atoi(ora.substr(ora.find(':') + 1).c_str());

    Veicolo elroq(skoda_elroq_85());
    auto percorso = cattolica_milano();
    const auto& sc = elroq.scheda();
    std::cout << std::fixed;

    std::cout << "ELECTRIC NAV - pianificatore di viaggio per auto elettriche\n\n";

    // --- Scheda auto ---------------------------------------------------
    std::cout << "=== " << sc.nome << " | motore " << sc.motore << " "
              << (int)sc.potenza_motore_kw << " kW ===\n"
              << "  Batteria " << (int)sc.batteria_lorda_kwh << " kWh lordi / "
              << (int)sc.batteria_netta_kwh << " netti | DC max " << (int)sc.potenza_dc_max_kw
              << " kW | Cx " << std::setprecision(2) << sc.cx << "\n";
    std::cout << "  Ricarica 10->80% su colonnina 300 kW: " << std::setprecision(0)
              << elroq.minuti_ricarica(10, 80, 300) << " min\n\n";

    // --- Consumo e autonomia ------------------------------------------
    std::cout << "Consumo e autonomia a " << std::setprecision(0) << par.cond.temperatura_c
              << " C (" << par.cond.passeggeri << " persone)\n"
              << "  km/h   kWh/100km   autonomia 100%   autonomia 80->10%\n";
    for (double v : {90, 100, 110, 120, 130}) {
        Condizioni c = par.cond; c.velocita_kmh = v;
        double k = elroq.consumo_100km(c);
        std::cout << std::setw(6) << v << std::setw(12) << std::setprecision(1) << k
                  << std::setw(14) << std::setprecision(0) << elroq.batteria() / k * 100 << " km"
                  << std::setw(16) << elroq.batteria() * 0.7 / k * 100 << " km\n";
    }

    // --- Piano ottimale ------------------------------------------------
    Piano p = Pianificatore(elroq, percorso, par).ottimizza();
    std::cout << "\n=== Viaggio " << percorso.front().nome << " -> " << percorso.back().nome
              << " (" << (int)percorso.back().km << " km) ===\n"
              << "  " << std::setprecision(0) << par.cond.velocita_kmh << " km/h | "
              << par.cond.temperatura_c << " C | partenza " << par.soc_partenza
              << "% alle " << ora << " | obiettivo "
              << (par.obiettivo == Obiettivo::Tempo ? "minimo tempo" : "minimo costo") << "\n\n";

    if (!p.fattibile) {
        std::cout << "  Impossibile arrivare con questi parametri.\n";
        return 1;
    }

    double ora_att = t0, km_att = 0;
    std::cout << "  " << orario(ora_att) << "  Partenza da " << percorso.front().nome
              << " con " << par.soc_partenza << "%\n";
    for (const auto& s : p.soste) {
        const Tappa& t = percorso[s.tappa];
        ora_att += (t.km - km_att) / par.cond.velocita_kmh * 60;
        km_att = t.km;
        std::cout << "  " << orario(ora_att) << "  " << t.nome << " (km " << (int)t.km
                  << ", " << (int)t.kw << " kW): arrivo " << std::setprecision(1)
                  << s.soc_arrivo << "% -> carico fino a " << s.soc_partenza << "%\n"
                  << "         " << std::setprecision(0) << s.minuti << " min, "
                  << std::setprecision(1) << s.kwh_rete << " kWh, "
                  << std::setprecision(2) << s.costo << " euro\n";
        ora_att += s.minuti;
        std::cout << "  " << orario(ora_att) << "  Ripartenza\n";
    }
    ora_att += (percorso.back().km - km_att) / par.cond.velocita_kmh * 60;
    std::cout << "  " << orario(ora_att) << "  Arrivo a " << percorso.back().nome
              << " con " << std::setprecision(1) << p.soc_arrivo << "%\n\n";

    std::cout << "  Guida " << durata(p.ore_guida) << " + soste " << durata(p.ore_ricarica)
              << " = TOTALE " << durata(p.ore_totali()) << "\n"
              << "  Energia consumata " << std::setprecision(1) << p.kwh_consumati << " kWh ("
              << p.kwh_consumati / percorso.back().km * 100 << " kWh/100km)"
              << " | spesa ricariche " << std::setprecision(2) << p.costo << " euro\n";

    // --- Confronto velocita x temperatura ------------------------------
    std::cout << "\n=== Tempo totale (soste) al variare di velocita e temperatura ===\n"
              << "  temp \\ km/h";
    std::vector<double> vel = {100, 110, 120, 130};
    for (double v : vel) std::cout << std::setw(13) << (int)v;
    std::cout << "\n";
    for (double temp : {-5.0, 5.0, 15.0, 25.0, 35.0}) {
        std::cout << std::setw(10) << std::setprecision(0) << temp << " C";
        for (double v : vel) {
            Parametri q = par; q.cond.velocita_kmh = v; q.cond.temperatura_c = temp;
            Piano r = Pianificatore(elroq, percorso, q).ottimizza();
            std::ostringstream cella;
            if (r.fattibile) cella << durata(r.ore_totali()) << " (" << r.soste.size() << ")";
            else cella << "--";
            std::cout << std::setw(13) << cella.str();
        }
        std::cout << "\n";
    }
    std::cout << "  (tra parentesi il numero di soste)\n";
    return 0;
}
