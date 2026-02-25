# Fan-Coil Raumbedieneinheit

Eine vollständige Raumbedieneinheit für Fan-Coil-Anlagen auf Basis von **ESP32**, **Nextion Touchdisplay** und **Modbus RTU (RS485)**.

## Hardware

| Komponente      | Bezeichnung                  | Beschreibung                          |
|-----------------|------------------------------|---------------------------------------|
| Mikrocontroller | ESP32 DevKit                 | Hauptsteuerung, WLAN/BT optional      |
| Display         | Nextion NX8048P070-011C      | 7", 800×480, kapazitiver Touchscreen  |
| Kommunikation   | RS485 (MAX485 / SP3485)      | Modbus RTU, 9600 Baud                 |
| Temperatursensor | DS18B20 oder NTC 10kΩ       | Lokale Raumtemperaturmessung          |

## Funktionen

- **Ist-Temperaturanzeige** – Lokaler Sensor (DS18B20/NTC) oder Modbus-Quelle
- **Solltemperatur** – Einstellbar 18–28°C über Touchscreen oder Modbus
- **Lüfterstufen** – Auto / Stufe 1 / Stufe 2 / Stufe 3
- **Betriebsmodi** – Heizen / Kühlen / Auto / Aus
- **Ventilstellung** – 0–100% Anzeige
- **Eco-Modus** – Automatische Temperaturabsenkung (konfigurierbar)
- **Frostschutz** – Minimale Temperatur immer gewährleistet
- **Zeitprogramm** – Wochenprogramm mit 4 Zeitfenstern pro Tag (Komfort/Eco/Aus)
- **Modbus Dual-Rolle** – ESP32 agiert gleichzeitig als Master und Slave
- **NVS-Speicherung** – Einstellungen bleiben nach Neustart erhalten
- **Watchdog Timer** – Für hohe Betriebssicherheit

## Architektur

```
┌─────────────────────┐      Serial       ┌─────────┐    RS485/Modbus    ┌──────────────────┐
│  Nextion Display    │◄────────────────►  │  ESP32  │◄──────────────────►│ Übergeordneter   │
│  NX8048P070-011C    │   UART (TX/RX)     │         │    RTU             │ Regler (SPS/DDC) │
│                     │                    │         │                    │                  │
│ - Sollwert          │                    │ - Logik │                    │ - Fan-Coil       │
│ - Lüfterstufe       │                    │ - Modbus│                    │   Steuerung      │
│ - Modus             │                    │ - RS485 │                    │                  │
│ - Ist-Temperatur    │                    │         │                    │                  │
└─────────────────────┘                    └─────────┘                    └──────────────────┘
```

## Dateistruktur

```
src/
├── main.cpp                    # Hauptprogramm ESP32 (Setup, Loop, Task-Koordination)
├── config.h                    # Pin-Konfiguration, Modbus-Einstellungen, Default-Werte
├── nextion_display.h           # Header: Nextion Display Kommunikation
├── nextion_display.cpp         # Nextion Display lesen/schreiben
├── modbus_handler.h            # Header: Modbus RTU Master & Slave Handler
├── modbus_handler.cpp          # Modbus RTU: Register lesen/schreiben
├── fancoil_controller.h        # Header: Fan-Coil Steuerungslogik
├── fancoil_controller.cpp      # Steuerungslogik: Modus, Lüfterstufe, Ventil, Frostschutz
├── scheduler.h                 # Header: Zeitprogramm/Wochenprogramm
├── scheduler.cpp               # Zeitprogramm: Wochentage, Zeitfenster, Komfort/Eco/Aus
├── register_map.h              # Modbus Register-Map Definition
docs/
├── MODBUS_REGISTER_MAP.md      # Dokumentation aller Modbus-Register
├── NEXTION_HMI_GUIDE.md        # Anleitung zum Erstellen des HMI-Layouts im Nextion Editor
├── WIRING_DIAGRAM.md           # Verdrahtungsplan ESP32 ↔ Nextion ↔ RS485
platformio.ini                  # PlatformIO Konfiguration für ESP32
README.md                       # Projektbeschreibung
```

## Schnellstart

### Voraussetzungen

- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- [Nextion Editor](https://nextion.tech/nextion-editor/)

### Projekt öffnen und bauen

```bash
# Repository klonen
git clone https://github.com/Jonnay0310/fancoil-raumbedieneinheit.git
cd fancoil-raumbedieneinheit

# Mit PlatformIO bauen und hochladen
pio run --target upload

# Seriellen Monitor öffnen
pio device monitor
```

### Konfiguration anpassen

Alle Pin-Definitionen und Standardwerte können in `src/config.h` angepasst werden:

```cpp
// Nextion Display – Serial2
#define NEXTION_RX_PIN    16
#define NEXTION_TX_PIN    17

// RS485 / Modbus – Serial1
#define MODBUS_TX_PIN     25
#define MODBUS_RX_PIN     26
#define MODBUS_DE_RE_PIN  27

// DS18B20 OneWire Sensor
#define ONEWIRE_PIN       4
```

## Modbus Registerübersicht

### Holding Register (R/W)

| Adresse | Beschreibung                  | Standard |
|---------|-------------------------------|----------|
| 0x0000  | Solltemperatur (×10, z.B. 220=22°C) | 220 |
| 0x0001  | Lüfterstufe (0=Auto, 1–3)     | 0        |
| 0x0002  | Betriebsmodus (0=Aus, 1=Heizen, 2=Kühlen, 3=Auto) | 0 |
| 0x0003  | Eco-Modus (0=Aus, 1=Ein)      | 0        |
| 0x0004  | Temperaturquelle (0=Lokal, 1=Modbus) | 0  |
| 0x0005  | Slave-Adresse (1–247)         | 1        |
| 0x0006  | Eco-Absenkung (×10)           | 20       |
| 0x0007  | Frostschutz-Temperatur (×10)  | 50       |

### Input Register (Read Only)

| Adresse | Beschreibung                  |
|---------|-------------------------------|
| 0x0010  | Ist-Temperatur (×10)          |
| 0x0011  | Ventilstellung (0–100%)       |
| 0x0012  | Aktive Lüfterstufe            |
| 0x0013  | Fehlerstatus (Bitfeld)        |
| 0x0014  | Aktueller Modus (effektiv)    |
| 0x0015  | Frostschutz aktiv (0/1)       |

Vollständige Dokumentation: [docs/MODBUS_REGISTER_MAP.md](docs/MODBUS_REGISTER_MAP.md)

## Dokumentation

- [Modbus Register-Map](docs/MODBUS_REGISTER_MAP.md) – Alle Register mit Beschreibung und Beispielen
- [Nextion HMI-Anleitung](docs/NEXTION_HMI_GUIDE.md) – Display-Layout und Objekt-IDs
- [Verdrahtungsplan](docs/WIRING_DIAGRAM.md) – Pin-Belegung und Anschlussschema

## Verwendete Bibliotheken

| Bibliothek                     | Version | Zweck                         |
|--------------------------------|---------|-------------------------------|
| `emelianov/modbus-esp8266`     | ^4.1.0  | Modbus RTU Master & Slave     |
| `paulstoffregen/OneWire`       | ^2.3.7  | OneWire-Protokoll (DS18B20)   |
| `milesburton/DallasTemperature` | ^3.11.0 | DS18B20 Temperatursensor     |

## Lizenz

Dieses Projekt steht unter der MIT-Lizenz.