# Modbus Register-Map – Fan-Coil Raumbedieneinheit

## Übersicht

Der ESP32 agiert gleichzeitig als **Modbus RTU Slave** (empfängt Befehle vom übergeordneten Regler) und als **Modbus RTU Master** (fragt optional die Ist-Temperatur ab).

- **Protokoll:** Modbus RTU
- **Baudrate:** 9600 Baud, 8N1
- **Standard Slave-Adresse:** 1 (konfigurierbar über Register 0x0005)
- **Schnittstelle:** RS485

---

## Holding Register (R/W) – Adressbereich 0x0000–0x0007

Holding Register können vom übergeordneten Regler (SPS/DDC) **gelesen und geschrieben** werden. Sie steuern die Betriebsparameter des Fan-Coil-Geräts.

| Adresse (hex) | Adresse (dez) | Name             | Beschreibung                                      | Einheit      | Wertebereich | Standard |
|---------------|---------------|------------------|---------------------------------------------------|--------------|--------------|----------|
| 0x0000        | 0             | Solltemperatur   | Gewünschte Raumtemperatur (Skalierung ×10)        | °C ×10       | 180–280      | 220      |
| 0x0001        | 1             | Lüfterstufe      | Gewünschte Lüfterstufe                            | –            | 0–3          | 0        |
| 0x0002        | 2             | Betriebsmodus    | Betriebsmodus des Geräts                          | –            | 0–3          | 0        |
| 0x0003        | 3             | Eco-Modus        | Energiesparmodus aktivieren                       | –            | 0–1          | 0        |
| 0x0004        | 4             | Temperaturquelle | Quelle der Ist-Temperatur                         | –            | 0–1          | 0        |
| 0x0005        | 5             | Slave-Adresse    | Modbus Slave-Adresse des ESP32                    | –            | 1–247        | 1        |
| 0x0006        | 6             | Eco-Absenkung    | Temperaturabsenkung im Eco-Modus (Skalierung ×10) | °C ×10       | 10–50        | 20       |
| 0x0007        | 7             | Frostschutz-Temp | Minimale Temperatur (Frostschutz) (Skalierung ×10) | °C ×10      | 30–100       | 50       |

### Lüfterstufen (Register 0x0001)

| Wert | Bedeutung        |
|------|------------------|
| 0    | Auto (automatisch) |
| 1    | Stufe 1 (niedrig) |
| 2    | Stufe 2 (mittel)  |
| 3    | Stufe 3 (hoch)    |

### Betriebsmodi (Register 0x0002)

| Wert | Bedeutung |
|------|-----------|
| 0    | Aus       |
| 1    | Heizen    |
| 2    | Kühlen    |
| 3    | Auto      |

### Temperaturquelle (Register 0x0004)

| Wert | Bedeutung                                     |
|------|-----------------------------------------------|
| 0    | Lokal (DS18B20 oder NTC am ESP32)             |
| 1    | Modbus (Abfrage beim übergeordneten Regler)   |

---

## Input Register (Read Only) – Adressbereich 0x0010–0x0015

Input Register können vom übergeordneten Regler nur **gelesen** werden. Sie liefern Statuswerte des Fan-Coil-Systems.

| Adresse (hex) | Adresse (dez) | Name                  | Beschreibung                                     | Einheit  | Wertebereich      |
|---------------|---------------|-----------------------|--------------------------------------------------|----------|-------------------|
| 0x0010        | 16            | Ist-Temperatur        | Aktuelle Raumtemperatur (Skalierung ×10)         | °C ×10   | -200–500          |
| 0x0011        | 17            | Ventilstellung        | Aktuelle Öffnung des Stellventils                | %        | 0–100             |
| 0x0012        | 18            | Aktive Lüfterstufe    | Tatsächlich aktive Lüfterstufe                   | –        | 0–3               |
| 0x0013        | 19            | Fehlerstatus          | Bitfeld mit aktiven Fehler-Flags                 | Bitfeld  | 0x0000–0xFFFF     |
| 0x0014        | 20            | Aktueller Modus       | Effektiver Betriebsmodus (inkl. Frostschutz)     | –        | 0–3               |
| 0x0015        | 21            | Frostschutz aktiv     | Frostschutz-Funktion ist aktiv                   | –        | 0=Nein, 1=Ja      |

---

## Fehlerstatus-Bits (Register 0x0013)

Der Fehlerstatus wird als **Bitfeld** übertragen. Mehrere Fehler können gleichzeitig aktiv sein.

| Bit | Wert  | Bedeutung                        |
|-----|-------|----------------------------------|
| 0   | 0x0001 | Temperatursensor-Fehler         |
| 1   | 0x0002 | Modbus-Kommunikationsfehler     |
| 2   | 0x0004 | Display-Kommunikationsfehler    |
| 3   | 0x0008 | NVS/EEPROM-Fehler               |
| 4–15 | –    | Reserviert                      |

---

## Beispiel-Kommunikation

### Solltemperatur auf 21.5°C setzen (Register 0x0000 = 215):

**Anfrage (Master → Slave):**
```
01 06 00 00 00 D7 89 CA
```
- `01` = Slave-Adresse
- `06` = Funktionscode (Write Single Register)
- `00 00` = Registeradresse (0x0000)
- `00 D7` = Wert (215 dezimal = 21.5°C ×10)
- `89 CA` = CRC16

**Antwort (Slave → Master):**
```
01 06 00 00 00 D7 89 CA
```
(Echo der Anfrage bei erfolgreichem Schreiben)

---

### Ist-Temperatur lesen (Input Register 0x0010):

**Anfrage (Master → Slave):**
```
01 04 00 10 00 01 C0 0C
```
- `01` = Slave-Adresse
- `04` = Funktionscode (Read Input Registers)
- `00 10` = Startadresse (0x0010)
- `00 01` = Anzahl Register
- `C0 0C` = CRC16

**Antwort (Slave → Master) – Beispiel: 23.5°C (235 = 0x00EB):**
```
01 04 02 00 EB 79 07
```

---

### Alle Input Register lesen (0x0010–0x0015):

**Anfrage:**
```
01 04 00 10 00 06 70 0D
```

**Antwort (Beispiel):**
```
01 04 0C 00 EB 00 32 00 02 00 00 00 01 00 00 xx xx
```
- `00 EB` = Ist-Temp: 235 → 23.5°C
- `00 32` = Ventil: 50%
- `00 02` = Lüfterstufe: 2
- `00 00` = Fehler: kein Fehler
- `00 01` = Modus: Heizen
- `00 00` = Frostschutz: inaktiv

---

## Modbus Funktionscodes

| Code | Funktion                    | Registertyp    |
|------|-----------------------------|----------------|
| 0x03 | Read Holding Registers      | Holding        |
| 0x04 | Read Input Registers        | Input          |
| 0x06 | Write Single Register       | Holding        |
| 0x10 | Write Multiple Registers    | Holding        |

---

## Hinweise

- Alle Temperaturwerte werden mit dem Faktor **×10** übertragen (Festkomma), um eine Dezimalstelle darzustellen.
  - Beispiel: 235 = 23.5°C
  - Beispiel: -15 = -1.5°C (als vorzeichenbehafteter 16-Bit-Wert: 0xFFF1)
- Die **Slave-Adresse** (Register 0x0005) wird erst nach einem Neustart des ESP32 aktiv.
- Im **Eco-Modus** wird die Solltemperatur intern um die eingestellte Eco-Absenkung reduziert.
- Der **Frostschutz** ist immer aktiv und übersteuert den Betriebsmodus „Aus", sobald die Temperatur unter die Frostschutz-Temperatur fällt.
