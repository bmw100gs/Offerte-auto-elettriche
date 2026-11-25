# Electric Nav

Pianificatore di viaggio per auto elettriche: calcola dove fermarsi a ricaricare,
fino a che percentuale e quanto si spende, sul percorso stradale reale.

Auto di riferimento: **Skoda Elroq 85** (motore APP550, batteria 77 kWh netti).
Percorso di esempio: **Cattolica → Milano** (A14 + A1).

## Cosa c'è nel repository

| Cartella | Contenuto |
|---|---|
| `web/index.html` | Interfaccia da usare nel browser, anche da telefono |
| `cpp/viaggio/` | Pianificatore in C++ (modello fisico + programmazione dinamica) |
| `cpp/viaggio/strumenti/scarica_percorso.py` | Scarica percorso, altitudini e colonnine |
| `cpp/viaggio/strumenti/genera_dati.py` | Aggiorna le tappe di `dati.h` dal CSV |
| `cpp/viaggio/percorsi/` | Percorsi già scaricati (CSV) |
| `cpp/ricarica.cpp` | Primo esempio: calcolatore di ricarica |

## Uso del programma C++

```sh
cd cpp/viaggio
make
./viaggio velocita=120 temp=15 partenza=90 arrivo=10 passeggeri=1 ora=08:30
./viaggio obiettivo=costo
```

## Fonti dei dati

- Percorso: OpenStreetMap tramite OSRM (licenza ODbL)
- Altitudini: EU-DEM tramite OpenTopoData
- Colonnine Free To X: dati pubblici di Autostrade per l'Italia
- Supercharger Tesla aperti a tutte le auto: supercharge.info
- IONITY: Open Charge Map (serve una chiave API, non ancora attiva)
- Dati auto: schede Skoda, ev-database, evkx (massa, Cx e rendimenti sono stime)

I prezzi al kWh sono stimati.
