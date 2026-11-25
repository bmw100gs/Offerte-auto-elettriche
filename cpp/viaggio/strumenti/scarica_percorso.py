#!/usr/bin/env python3
"""Scarica da OpenStreetMap un percorso reale e lo salva in CSV per il pianificatore.

Fonti (gratuite, senza chiave):
  - OSRM        percorso stradale e velocita tipiche per tratto
  - OpenTopoData altitudine (modello EU-DEM 25 m)
  - Autostrade per l'Italia (viabilita.autostrade.it) colonnine Free To X
                 ufficiali nelle aree di servizio, con direzione e potenza
  - supercharge.info  Supercharger Tesla (solo quelli aperti a tutte le auto)
  - Open Charge Map  colonnine IONITY (serve una chiave API, iniettata dal proxy)
  - Overpass    colonnine mappate su OpenStreetMap (riserva)

Uso:  python3 scarica_percorso.py Cattolica Milano percorsi/cattolica_milano.csv
      (citta note nel dizionario sotto, oppure coordinate "lat,lon")
"""
import json
import math
import sys
import time
import urllib.parse
import urllib.request

CITTA = {
    "cattolica": (43.9625, 12.7389), "milano": (45.4642, 9.1900),
    "rimini": (44.0594, 12.5683), "bologna": (44.4949, 11.3426),
    "roma": (41.9028, 12.4964), "napoli": (40.8518, 14.2681),
    "firenze": (43.7696, 11.2558), "torino": (45.0703, 7.6869),
    "venezia": (45.4408, 12.3155), "verona": (45.4384, 10.9916),
    "ancona": (43.6158, 13.5189), "pesaro": (43.9098, 12.9131),
}
PASSO_KM = 2.0          # risoluzione del profilo altimetrico
RAGGIO_COLONNINE_M = 400
MAX_DEVIAZIONE_M = 6000   # stazioni fuori percorso: distanza massima in linea d'aria


def coord(nome):
    if "," in nome:
        lat, lon = nome.split(",")
        return float(lat), float(lon)
    return CITTA[nome.lower()]


def scarica(url, dati=None, tentativi=4):
    for n in range(tentativi):
        try:
            req = urllib.request.Request(url, data=dati,
                                         headers={"User-Agent": "pianificatore-ev-didattico"})
            with urllib.request.urlopen(req, timeout=120) as r:
                return json.load(r)
        except Exception as e:  # rete instabile: riprova con attesa crescente
            if n == tentativi - 1:
                raise
            print(f"  ...errore ({e}), riprovo", file=sys.stderr)
            time.sleep(3 * (n + 1))


def distanza_m(a, b):
    lat1, lon1, lat2, lon2 = map(math.radians, (a[0], a[1], b[0], b[1]))
    h = (math.sin((lat2 - lat1) / 2) ** 2
         + math.cos(lat1) * math.cos(lat2) * math.sin((lon2 - lon1) / 2) ** 2)
    return 6371000 * 2 * math.asin(math.sqrt(h))


def percorso_osrm(da, a):
    url = (f"https://router.project-osrm.org/route/v1/driving/"
           f"{da[1]},{da[0]};{a[1]},{a[0]}?overview=full&geometries=geojson&annotations=true")
    r = scarica(url)["routes"][0]
    punti = [(lat, lon) for lon, lat in r["geometry"]["coordinates"]]
    ann = r["legs"][0]["annotation"]
    return punti, ann["distance"], ann["duration"]


def ricampiona(punti, dist, dur):
    """Profilo a passi regolari: (km, lat, lon, velocita media OSRM km/h)."""
    profilo, km, acc_d, acc_t = [(0.0, *punti[0], 0.0)], 0.0, 0.0, 0.0
    for i, (d, t) in enumerate(zip(dist, dur)):
        km += d / 1000
        acc_d += d
        acc_t += t
        if acc_d >= PASSO_KM * 1000 or i == len(dist) - 1:
            vel = acc_d / acc_t * 3.6 if acc_t > 0 else 50
            profilo.append((km, *punti[i + 1], vel))
            acc_d = acc_t = 0.0
    profilo[0] = (0.0, *punti[0], profilo[1][3])
    return profilo


def altitudini(profilo):
    quote = []
    for i in range(0, len(profilo), 100):  # max 100 punti per richiesta
        blocco = "|".join(f"{p[1]:.5f},{p[2]:.5f}" for p in profilo[i:i + 100])
        r = scarica("https://api.opentopodata.org/v1/eudem25m?locations=" + blocco)
        quote += [x["elevation"] or 0.0 for x in r["results"]]
        time.sleep(1.1)  # limite: 1 richiesta al secondo
    return quote


def kw_da_tag(tag):
    """Potenza massima dichiarata nei tag OSM (es. '150 kW', '50')."""
    best = 0.0
    for k, v in tag.items():
        if "output" in k or k in ("maxpower", "charging_station:output"):
            for parte in v.replace(",", ";").split(";"):
                num = "".join(c for c in parte if c.isdigit() or c == ".")
                if num:
                    x = float(num)
                    if "w" in parte.lower() and "kw" not in parte.lower():
                        x /= 1000
                    best = max(best, x)
    return best


def proietta(punti, km_cum, lat, lon):
    """Proietta (lat, lon) sulla geometria del percorso.
    Restituisce (distanza m, km lungo il percorso, True se sta a destra).
    In Italia si guida a destra: destra = carreggiata nella nostra direzione."""
    best = None
    cl = math.cos(math.radians(lat))
    for k in range(len(punti) - 1):
        (ya, xa), (yb, xb) = punti[k], punti[k + 1]
        # coordinate locali in metri
        ax, ay = (xa - lon) * cl * 111320, (ya - lat) * 110540
        bx, by = (xb - lon) * cl * 111320, (yb - lat) * 110540
        dx, dy = bx - ax, by - ay
        l2 = dx * dx + dy * dy
        t = 0 if l2 == 0 else max(0, min(1, -(ax * dx + ay * dy) / l2))
        px, py = ax + t * dx, ay + t * dy
        d = math.hypot(px, py)
        if best is None or d < best[0]:
            destra = (dx * (-ay) - dy * (-ax)) < 0   # punto (0,0) rispetto al segmento
            best = (d, km_cum[k] + t * (km_cum[k + 1] - km_cum[k]), destra)
    return best


def colonnine_autostrade(punti, dist):
    """Colonnine ufficiali Free To X dal sito viabilita di Autostrade per l'Italia."""
    aree = scarica("https://viabilita.autostrade.it/json/colonnine.json")["listaAree"]
    km_cum = [0.0]
    for d in dist:
        km_cum.append(km_cum[-1] + d / 1000)
    trovate = []
    for a in aree:
        cols = a.get("colonnine") or []
        if not cols or a.get("lat") is None:
            continue
        d, km, destra = proietta(punti, km_cum, a["lat"], a["lon"])
        if d > 400 or not destra:       # lontana o sulla carreggiata opposta
            continue
        kw = 0
        for c in cols:
            num = "".join(ch for ch in c.get("modello", "").split("kW")[0] if ch.isdigit())
            kw = max(kw, int(num) if num else 0)
        if kw < 50:
            continue
        nome = f"{a['strada'].split()[0]} {a['nome'].title()} (Free To X)"
        prezzo = 0.79 if kw >= 150 else 0.69     # stima: prezzo non pubblicato nei dati
        trovate.append((km, nome, kw, prezzo, d))
    return sorted(trovate)


def km_cumulati(dist):
    km = [0.0]
    for d in dist:
        km.append(km[-1] + d / 1000)
    return km


def vicino_al_percorso(punti, km_cum, lat, lon):
    """(distanza m, km sul percorso) se la stazione e' raggiungibile, altrimenti None.
    Vicino all'autostrada (< 400 m) conta la carreggiata; piu' lontano si esce
    dall'autostrada e la carreggiata non conta."""
    d, km, destra = proietta(punti, km_cum, lat, lon)
    if d > MAX_DEVIAZIONE_M or (d <= RAGGIO_COLONNINE_M and not destra):
        return None
    return d, km


def colonnine_tesla(punti, dist):
    """Supercharger Tesla aperti anche alle altre marche (supercharge.info)."""
    siti = scarica("https://supercharge.info/service/supercharge/allSites")
    km_cum = km_cumulati(dist)
    lat_min = min(p[0] for p in punti) - 0.1
    lat_max = max(p[0] for p in punti) + 0.1
    lon_min = min(p[1] for p in punti) - 0.1
    lon_max = max(p[1] for p in punti) + 0.1
    trovate = []
    for x in siti:
        lat, lon = x["gps"]["latitude"], x["gps"]["longitude"]
        if not (lat_min < lat < lat_max and lon_min < lon < lon_max):
            continue
        if x.get("status") != "OPEN" or not x.get("otherEVs"):
            continue
        v = vicino_al_percorso(punti, km_cum, lat, lon)
        if v is None:
            continue
        kw = x.get("powerKilowatt") or 150
        nome = "Tesla " + x["name"].replace(", Italy", "").replace(",", " ")[:30]
        trovate.append((v[1], nome, kw, 0.69, v[0]))   # prezzo non-Tesla stimato
    return trovate


def colonnine_ionity(punti, dist, profilo):
    """Colonnine IONITY da Open Charge Map (operatore 3299)."""
    km_cum = km_cumulati(dist)
    # Open Charge Map accetta una polilinea codificata: usiamo il profilo ridotto
    def codifica(coords):
        out, plat, plon = [], 0, 0
        for lat, lon in coords:
            for v, prev in ((round(lat * 1e5), plat), (round(lon * 1e5), plon)):
                d = v - prev
                d = ~(d << 1) if d < 0 else d << 1
                while d >= 0x20:
                    out.append(chr((0x20 | (d & 0x1f)) + 63))
                    d >>= 5
                out.append(chr(d + 63))
            plat, plon = round(lat * 1e5), round(lon * 1e5)
        return "".join(out)
    linea = codifica([(p[1], p[2]) for p in profilo])
    url = ("https://api.openchargemap.io/v3/poi?operatorid=3299&maxresults=200&compact=true"
           f"&verbose=false&distanceunit=km&distance={MAX_DEVIAZIONE_M / 1000}"
           f"&polyline={urllib.parse.quote(linea)}")
    trovate = []
    for x in scarica(url):
        a = x.get("AddressInfo", {})
        lat, lon = a.get("Latitude"), a.get("Longitude")
        if lat is None or x.get("StatusTypeID") in (100, 150, 200):   # non operative
            continue
        v = vicino_al_percorso(punti, km_cum, lat, lon)
        if v is None:
            continue
        kw = max([c.get("PowerKW") or 0 for c in x.get("Connections") or []] + [0]) or 350
        nome = "IONITY " + (a.get("Town") or a.get("Title") or "").replace(",", " ")[:30]
        trovate.append((v[1], nome, kw, 0.69, v[0]))   # prezzo IONITY Direct stimato
    return trovate


def colonnine(profilo):
    linea = ",".join(f"{p[1]:.5f},{p[2]:.5f}" for p in profilo[::2])
    query = (f"[out:json][timeout:180];"
             f"nwr(around:{RAGGIO_COLONNINE_M},{linea})[amenity=charging_station];"
             f"out center tags;")
    dati = urllib.parse.urlencode({"data": query}).encode()
    r = None
    for server in ("overpass-api.de", "overpass.kumi.systems", "overpass.private.coffee"):
        try:
            r = scarica(f"https://{server}/api/interpreter", dati, tentativi=2)
            break
        except Exception as e:
            print(f"  {server} non risponde ({e})", file=sys.stderr)
    if r is None:
        print("  ATTENZIONE: colonnine non scaricate, solo percorso e quote", file=sys.stderr)
        return []
    trovate = []
    for el in r["elements"]:
        tag = el.get("tags", {})
        lat = el.get("lat", el.get("center", {}).get("lat"))
        lon = el.get("lon", el.get("center", {}).get("lon"))
        kw = kw_da_tag(tag)
        if kw < 50:          # solo ricarica rapida DC
            continue
        vicino = min(profilo, key=lambda p: distanza_m((lat, lon), (p[1], p[2])))
        nome = tag.get("name") or tag.get("operator") or tag.get("brand") or "Colonnina"
        if tag.get("operator") and tag.get("operator") not in nome:
            nome += " - " + tag["operator"]
        prezzo = 0.79 if kw >= 150 else 0.69   # OSM non ha i prezzi: stima
        trovate.append((vicino[0], nome.replace(",", " ")[:40], kw, prezzo,
                        distanza_m((lat, lon), (vicino[1], vicino[2]))))
    trovate.sort()
    return trovate


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    da, a, uscita = coord(sys.argv[1]), coord(sys.argv[2]), sys.argv[3]
    print("Percorso (OSRM)...", file=sys.stderr)
    punti, dist, dur = percorso_osrm(da, a)
    profilo = ricampiona(punti, dist, dur)
    print(f"  {profilo[-1][0]:.1f} km, {len(profilo)} punti", file=sys.stderr)
    print("Altitudini (OpenTopoData)...", file=sys.stderr)
    quote = altitudini(profilo)
    print("Colonnine (Autostrade per l'Italia)...", file=sys.stderr)
    try:
        cols = colonnine_autostrade(punti, dist)
    except Exception as e:
        print(f"  non disponibili ({e}), provo Overpass", file=sys.stderr)
        cols = colonnine(profilo)
    for nome, funz in (("Tesla Supercharger", lambda: colonnine_tesla(punti, dist)),
                       ("IONITY", lambda: colonnine_ionity(punti, dist, profilo))):
        print(f"Colonnine {nome}...", file=sys.stderr)
        try:
            extra = funz()
            print(f"  {len(extra)} trovate", file=sys.stderr)
            cols += extra
        except Exception as e:
            print(f"  non disponibili ({e})", file=sys.stderr)
    km_fine = profilo[-1][0]
    cols = sorted(c for c in cols if 1 < c[0] < km_fine - 1)
    print(f"  {len(cols)} colonnine DC >= 50 kW", file=sys.stderr)

    with open(uscita, "w") as f:
        f.write(f"# {sys.argv[1]} -> {sys.argv[2]} | percorso OpenStreetMap (ODbL), colonnine Autostrade per l Italia\n")
        f.write("# P,km,quota_m,velocita_osrm_kmh\n")
        for p, q in zip(profilo, quote):
            f.write(f"P,{p[0]:.2f},{q:.0f},{p[3]:.0f}\n")
        f.write("# C,km,nome,kw,prezzo_stimato,distanza_dal_percorso_m\n")
        for c in cols:
            f.write(f"C,{c[0]:.2f},{c[1]},{c[2]:.0f},{c[3]:.2f},{c[4]:.0f}\n")
        f.write(f"N,{sys.argv[1]},{sys.argv[2]}\n")
    print("Salvato in", uscita, file=sys.stderr)


if __name__ == "__main__":
    main()
