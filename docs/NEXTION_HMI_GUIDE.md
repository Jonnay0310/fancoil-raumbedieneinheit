# Nextion HMI-Layoutanleitung – Fan-Coil Raumbedieneinheit

## Hardware

- **Display:** Nextion NX8048P070-011C
- **Auflösung:** 800 × 480 Pixel
- **Display-Typ:** Intelligent Series (kapazitiver Touchscreen)
- **Verbindung:** UART (Serial2, GPIO16/17), 9600 Baud

---

## Allgemeine Einstellungen im Nextion Editor

1. Nextion Editor öffnen
2. Neues Projekt erstellen: `File → New`
3. Displaytyp auswählen: **NX8048P070-011C** (7.0", 800×480)
4. Ausrichtung: **Landscape (0°)**
5. Baudrate: **9600**

---

## Seite 0: Hauptbildschirm (page0)

### Beschreibung
Der Hauptbildschirm zeigt alle wichtigen Betriebsinformationen auf einen Blick.

### Layout (800 × 480 px)

```
┌─────────────────────────────────────────────────────────────────────┐
│  Fan-Coil Raumbedieneinheit                          ⚙  ECO  FROST  │
│                                                                       │
│          IST-TEMPERATUR              SOLLTEMPERATUR                   │
│          ┌──────────────┐            ┌──────────────┐                │
│          │   23.5°C     │            │   22.0°C     │                │
│          └──────────────┘            └──────────────┘                │
│                                                                       │
│    MODUS          LÜFTER          VENTIL        STATUS               │
│   ┌───────┐      ┌───────┐       ┌───────┐    ┌──────────┐          │
│   │HEIZEN │      │STUFE 2│       │  50%  │    │          │          │
│   └───────┘      └───────┘       └───────┘    └──────────┘          │
│                                                                       │
│  [  EINSTELLUNGEN  ]  [  ZEITPROGRAMM  ]  [  SYSTEM  ]              │
└─────────────────────────────────────────────────────────────────────┘
```

### Objekte (Nextion-Komponenten)

| Objekt-Name        | Typ       | ID  | Position (x,y) | Größe (w,h) | Beschreibung              |
|--------------------|-----------|-----|----------------|-------------|---------------------------|
| `t_title`          | Text      | 1   | 10, 5          | 600, 35     | Titel "Fan-Coil..."       |
| `t_eco`            | Text      | 2   | 650, 5         | 60, 30      | "ECO" wenn Eco aktiv      |
| `t_frost`          | Text      | 3   | 720, 5         | 70, 30      | "FROST" wenn Frost aktiv  |
| `t_actual_temp`    | Text      | 4   | 50, 80         | 300, 100    | Ist-Temperatur (groß)     |
| `t_setpoint`       | Text      | 5   | 430, 80        | 300, 100    | Solltemperatur (groß)     |
| `t_mode`           | Text      | 6   | 30, 250        | 160, 60     | Betriebsmodus             |
| `t_fan`            | Text      | 7   | 220, 250       | 160, 60     | Lüfterstufe               |
| `t_valve`          | Text      | 8   | 410, 250       | 160, 60     | Ventilstellung            |
| `t_error`          | Text      | 9   | 580, 250       | 190, 60     | Fehlerstatus (rot)        |
| `b_settings`       | Button    | 10  | 30, 380        | 220, 60     | → Seite 1 (Einstellungen) |
| `b_schedule`       | Button    | 11  | 290, 380       | 220, 60     | → Seite 2 (Zeitprogramm)  |
| `b_system`         | Button    | 12  | 550, 380       | 220, 60     | → Seite 3 (System)        |

### Schaltflächen-Ereignisse (Touch Events)

```
// b_settings – Seite 1 öffnen
page 1

// b_schedule – Seite 2 öffnen
page 2

// b_system – Seite 3 öffnen
page 3
```

---

## Seite 1: Einstellungen (page1)

### Beschreibung
Hier können Solltemperatur, Betriebsmodus, Lüfterstufe und Eco-Modus eingestellt werden.

### Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  Einstellungen                                          [← ZURÜCK]  │
│                                                                       │
│  SOLLTEMPERATUR                                                       │
│  ┌──────────────────────────────────────────────┐                   │
│  │   [-]          22.0°C          [+]           │                   │
│  └──────────────────────────────────────────────┘                   │
│                                                                       │
│  BETRIEBSMODUS                                                        │
│  [ AUS ]  [ HEIZEN ]  [ KÜHLEN ]  [ AUTO ]                          │
│                                                                       │
│  LÜFTERSTUFE                                                          │
│  [ AUTO ]  [ STUFE 1 ]  [ STUFE 2 ]  [ STUFE 3 ]                   │
│                                                                       │
│  ECO-MODUS: [ EIN/AUS ]                                              │
└─────────────────────────────────────────────────────────────────────┘
```

### Objekte

| Objekt-Name   | Typ    | ID  | Position (x,y) | Größe (w,h) | Beschreibung                        |
|---------------|--------|-----|----------------|-------------|-------------------------------------|
| `t_title`     | Text   | 1   | 10, 5          | 600, 35     | "Einstellungen"                     |
| `b_back`      | Button | 0   | 620, 5         | 160, 50     | ← Zurück zu Seite 0                 |
| `n_setpoint`  | Number | 14  | 320, 100       | 160, 60     | Solltemperatur (×10, nur Anzeige)   |
| `b_sp_minus`  | Button | 2   | 100, 100       | 100, 60     | [-] Verringern um 0.5°C             |
| `b_sp_plus`   | Button | 1   | 580, 100       | 100, 60     | [+] Erhöhen um 0.5°C               |
| `b_mode_off`  | Button | 3   | 30, 230        | 160, 60     | Modus: AUS                          |
| `b_mode_heat` | Button | 4   | 210, 230       | 160, 60     | Modus: HEIZEN                       |
| `b_mode_cool` | Button | 5   | 390, 230       | 160, 60     | Modus: KÜHLEN                       |
| `b_mode_auto` | Button | 6   | 570, 230       | 160, 60     | Modus: AUTO                         |
| `b_fan_auto`  | Button | 7   | 30, 340        | 160, 60     | Lüfter: AUTO                        |
| `b_fan_1`     | Button | 8   | 210, 340       | 160, 60     | Lüfter: STUFE 1                     |
| `b_fan_2`     | Button | 9   | 390, 340       | 160, 60     | Lüfter: STUFE 2                     |
| `b_fan_3`     | Button | 10  | 570, 340       | 160, 60     | Lüfter: STUFE 3                     |
| `b_eco`       | Button | 11  | 30, 420        | 200, 50     | Eco-Modus EIN/AUS                   |
| `n_eco`       | Number | 15  | 250, 425       | 60, 40      | Eco-Status (0=AUS, 1=AN)            |
| `n_mode`      | Number | 16  | 0, 0           | 1, 1        | Modus-Wert (versteckt, für Sync)    |
| `n_fan`       | Number | 17  | 0, 0           | 1, 1        | Lüfter-Wert (versteckt, für Sync)   |

### Schaltflächen-Ereignisse

```
// b_back – Zurück zur Hauptseite
page 0

// b_sp_plus – Solltemperatur erhöhen
// (Komponenten-ID 1, Seite 1 → ESP32 erkennt: Erhöhen-Signal)
// Kein zusätzlicher Code nötig, ESP32 wertet Touch-Ereignis aus

// b_mode_off (Komponenten-ID 3 auf Seite 1)
// b_mode_heat (Komponenten-ID 4 auf Seite 1)
// b_mode_cool (Komponenten-ID 5 auf Seite 1)
// b_mode_auto (Komponenten-ID 6 auf Seite 1)
// ESP32 empfängt: 0x65 [Seite=1] [KomponentenID] [1]
```

> **Wichtig:** Die Komponenten-IDs auf Seite 1 müssen exakt den in `nextion_display.cpp` definierten IDs entsprechen (b_sp_plus = ID 1, b_sp_minus = ID 2, Modus = ID 3–6, Lüfter = ID 7–10, Eco = ID 11).

---

## Seite 2: Zeitprogramm (page2)

### Beschreibung
Konfiguration des Wochenprogramms mit Zeitfenstern pro Tag.

### Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  Zeitprogramm                                           [← ZURÜCK]  │
│                                                                       │
│  TAG: [Mo] [Di] [Mi] [Do] [Fr] [Sa] [So]                            │
│                                                                       │
│  ZEITFENSTER   START       ENDE        MODUS                         │
│  Slot 1:     [ 06 ]:[ 00 ] – [ 22 ]:[ 00 ]  [KOMFORT]              │
│  Slot 2:     [ -- ]:[ -- ] – [ -- ]:[ -- ]  [  ---  ]              │
│  Slot 3:     [ -- ]:[ -- ] – [ -- ]:[ -- ]  [  ---  ]              │
│  Slot 4:     [ -- ]:[ -- ] – [ -- ]:[ -- ]  [  ---  ]              │
│                                                                       │
│                              [SPEICHERN]                              │
└─────────────────────────────────────────────────────────────────────┘
```

### Objekte

| Objekt-Name      | Typ    | ID  | Beschreibung                         |
|------------------|--------|-----|--------------------------------------|
| `b_back`         | Button | 0   | ← Zurück                             |
| `n_day`          | Number | 1   | Ausgewählter Tag (0–6)               |
| `b_day_0`–`b_day_6` | Button | 2–8 | Tagauswahl Mo–So                |
| `n_slot`         | Number | 9   | Aktives Zeitfenster (0–3)            |
| `n_start_h`      | Number | 10  | Startzeit Stunde (0–23)              |
| `n_start_m`      | Number | 11  | Startzeit Minute (0–59)              |
| `n_end_h`        | Number | 12  | Endzeit Stunde (0–23)                |
| `n_end_m`        | Number | 13  | Endzeit Minute (0–59)                |
| `n_sched_mode`   | Number | 14  | Zeitfenster-Modus (0/1/2)            |
| `b_save`         | Button | 15  | Einstellungen speichern              |

---

## Seite 3: Systemeinstellungen (page3)

### Beschreibung
Erweiterte Systemkonfiguration: Modbus-Adresse, Temperaturquelle, Eco-Absenkung, Frostschutz.

### Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  Systemeinstellungen                                    [← ZURÜCK]  │
│                                                                       │
│  MODBUS SLAVE-ADRESSE:   [ 001 ] (1–247)                            │
│                                                                       │
│  TEMPERATURQUELLE:        [LOKAL]  [MODBUS]                         │
│                                                                       │
│  ECO-ABSENKUNG:          [ 2.0°C ] (0.5–5.0°C)                     │
│                                                                       │
│  FROSTSCHUTZ-TEMPERATUR: [ 5.0°C ] (3.0–10.0°C)                   │
│                                                                       │
│  FIRMWARE-VERSION: v1.0.0                                            │
│                                [SPEICHERN]                           │
└─────────────────────────────────────────────────────────────────────┘
```

### Objekte

| Objekt-Name      | Typ    | ID  | Beschreibung                              |
|------------------|--------|-----|-------------------------------------------|
| `b_back`         | Button | 0   | ← Zurück                                  |
| `n_slave_addr`   | Number | 1   | Modbus Slave-Adresse (1–247)              |
| `b_addr_minus`   | Button | 2   | Adresse verringern                        |
| `b_addr_plus`    | Button | 3   | Adresse erhöhen                           |
| `n_temp_src`     | Number | 4   | Temperaturquelle (0=Lokal, 1=Modbus)     |
| `b_src_local`    | Button | 5   | Quelle: Lokal                             |
| `b_src_modbus`   | Button | 6   | Quelle: Modbus                            |
| `n_eco_offset`   | Number | 7   | Eco-Absenkung (×10)                       |
| `b_eco_minus`    | Button | 8   | Eco-Absenkung verringern                  |
| `b_eco_plus`     | Button | 9   | Eco-Absenkung erhöhen                     |
| `n_frost_temp`   | Number | 10  | Frostschutz-Temperatur (×10)             |
| `b_frost_minus`  | Button | 11  | Frostschutz-Temp. verringern              |
| `b_frost_plus`   | Button | 12  | Frostschutz-Temp. erhöhen                |
| `b_save`         | Button | 13  | Alle Einstellungen speichern              |
| `t_version`      | Text   | 14  | Firmware-Version                          |

---

## Nextion Protokoll-Referenz

### Befehl vom ESP32 an Display senden

Alle Befehle enden mit drei `0xFF`-Bytes:

```
// Text setzen
t_actual_temp.txt="23.5°C"[0xFF][0xFF][0xFF]

// Zahl setzen
n_setpoint.val=220[0xFF][0xFF][0xFF]

// Seite wechseln
page 1[0xFF][0xFF][0xFF]
```

### Ereignisse vom Display an ESP32

Das Display sendet Binärdaten:

| Byte 0 | Bedeutung            | Weitere Bytes                         |
|--------|----------------------|---------------------------------------|
| 0x65   | Touch-Ereignis       | [Seite][KomponentenID][Ereignis(0/1)] |
| 0x66   | Seite geöffnet       | [Seiten-ID]                           |
| 0x71   | Numerischer Wert     | [Val0][Val1][Val2][Val3] (LE, 32-Bit) |
| 0x70   | Textwert             | [Text-Bytes...]                       |

Alle Ereignis-Nachrichten enden mit drei `0xFF`-Bytes.

---

## Nextion Editor – Tipps

1. **Schriftarten:** Vorgefertigte Schriften über `Tools → Font Generator` erstellen
   - Empfohlen: Arial 32pt für Temperaturen, 16pt für Labels
2. **Farben:** Hintergrund schwarz (0x0000), Text weiß (0xFFFF) für guten Kontrast
3. **HMI-Datei kompilieren:** `Compile → Compile` (`.tft`-Datei erzeugen)
4. **Upload auf Display:**
   - MicroSD-Karte mit FAT32 formatieren
   - `.tft`-Datei in den Root-Ordner kopieren
   - Display mit eingelegter SD-Karte einschalten
   - Automatische Aktualisierung startet
5. **UART Test:** Nextion Debug-Konsole in Editor nutzen (`Debug → Open`)
