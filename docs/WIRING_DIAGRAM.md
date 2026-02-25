# Verdrahtungsplan – Fan-Coil Raumbedieneinheit

## Übersicht

```
┌─────────────────────┐      Serial       ┌─────────┐    RS485/Modbus    ┌──────────────────┐
│  Nextion Display    │◄────────────────►  │  ESP32  │◄──────────────────►│ Übergeordneter   │
│  NX8048P070-011C    │   UART (TX/RX)     │         │    RTU             │ Regler (SPS/DDC) │
│                     │                    │         │                    │                  │
│ 4-poliger Stecker:  │                    │ Logik   │                    │ Fan-Coil         │
│ +5V, GND, TX, RX   │                    │ Modbus  │                    │ Steuerung        │
│                     │                    │ RS485   │                    │                  │
└─────────────────────┘                    └────┬────┘                    └──────────────────┘
                                                │
                                           DS18B20 oder NTC
                                           (Temperatursensor)
```

---

## ESP32 Pinbelegung

| ESP32 GPIO | Funktion               | Verbunden mit              | Beschreibung                        |
|------------|------------------------|----------------------------|-------------------------------------|
| GPIO 16    | UART2 RX               | Nextion TX (Pin 4)         | Empfang vom Nextion Display         |
| GPIO 17    | UART2 TX               | Nextion RX (Pin 3)         | Senden an Nextion Display           |
| GPIO 25    | UART1 TX               | RS485 Modul DI (Driver In) | Modbus RTU Senden                   |
| GPIO 26    | UART1 RX               | RS485 Modul RO (Receiver Out) | Modbus RTU Empfangen             |
| GPIO 27    | RS485 DE/RE            | RS485 Modul DE + RE        | Sende-/Empfangumschaltung           |
| GPIO 4     | OneWire                | DS18B20 Data               | Temperatursensor OneWire            |
| GPIO 34    | Analog Input (ADC)     | NTC Spannungsteiler        | Alternativer NTC-Sensor             |
| 3.3V       | Versorgung             | DS18B20 VCC                | Sensor-Versorgung                   |
| GND        | Masse                  | Alle GND-Verbindungen      | Gemeinsame Masse                    |
| 5V         | Versorgung             | Nextion +5V                | Display-Versorgung                  |

> **Hinweis:** GPIO 34 ist nur Input und hat keinen internen Pull-Up. Er eignet sich gut für analoge Messungen.

---

## Nextion Display (NX8048P070-011C) Anschluss

Das Nextion NX8048P070-011C hat einen **4-poligen JST-Stecker**:

| Nextion Pin | Farbe  | ESP32 Pin | Beschreibung         |
|-------------|--------|-----------|----------------------|
| 1 – +5V     | Rot    | 5V        | Versorgungsspannung  |
| 2 – GND     | Schwarz | GND      | Masse                |
| 3 – RX      | Gelb   | GPIO 17   | Daten von ESP32      |
| 4 – TX      | Blau   | GPIO 16   | Daten an ESP32       |

> **Wichtig:** Das Nextion Display benötigt **5V**. Die UART-Leitungen (3.3V vom ESP32) sind direkt kompatibel, da der Nextion 3.3V-Pegel akzeptiert.

---

## RS485 Modul (z.B. MAX485 oder SP3485)

### Typisches RS485-Modul (4/5-poliger Stecker):

| RS485 Modul Pin | Verbunden mit    | Beschreibung                                |
|-----------------|------------------|---------------------------------------------|
| VCC (3.3V)      | ESP32 3.3V       | Modulversorgung                             |
| GND             | ESP32 GND        | Masse                                       |
| DI (Driver In)  | ESP32 GPIO 25    | Sendedaten (TX vom ESP32)                   |
| RO (Receiver Out) | ESP32 GPIO 26  | Empfangsdaten (RX zum ESP32)                |
| DE (Driver Enable) | ESP32 GPIO 27 | Sende-Aktivierung (HIGH = Senden)           |
| RE (Receiver Enable) | ESP32 GPIO 27 | Empfangs-Aktivierung (LOW = Empfangen)    |

> **Hinweis:** DE und RE werden gemeinsam an GPIO 27 angeschlossen (DE und RE verbinden). Die Modbus-Bibliothek schaltet diesen Pin automatisch um.

### RS485 Bus-Anschluss:

| RS485 Modul | RS485 Bus        | Beschreibung          |
|-------------|------------------|-----------------------|
| A (+)       | Bus A (+)        | Differenzleitung A    |
| B (-)       | Bus B (-)        | Differenzleitung B    |
| GND         | (optional) Bus GND | Schirmung / Bezug   |

---

## DS18B20 Temperatursensor

### Anschluss (3-poliger Sensor):

| DS18B20 Pin | Verbunden mit      | Beschreibung              |
|-------------|--------------------|---------------------------|
| VDD (+)     | ESP32 3.3V         | Versorgungsspannung       |
| GND (-)     | ESP32 GND          | Masse                     |
| DQ (Data)   | ESP32 GPIO 4       | OneWire Datenleitung      |

> **Pull-Up Widerstand:** Zwischen DQ und VDD einen **4.7kΩ** Widerstand einbauen (erforderlich für OneWire-Protokoll).

### Anschluss-Schema:

```
ESP32 3.3V ──┬──── DS18B20 VDD
             │
           4.7kΩ
             │
ESP32 GPIO 4 ┴──── DS18B20 DQ
                       │
ESP32 GND ─────────── DS18B20 GND
```

---

## NTC Temperatursensor (Alternative)

### Anschluss (Spannungsteiler):

```
ESP32 3.3V ─────── 10kΩ Widerstand ─────┬───── ESP32 GPIO 34 (ADC)
                                          │
                                        NTC 10kΩ (bei 25°C)
                                          │
ESP32 GND ────────────────────────────────┘
```

| Bauteil            | Wert   | Beschreibung                             |
|--------------------|--------|------------------------------------------|
| Vorwiderstand R1   | 10kΩ   | Festwiderstand (Serie mit NTC)           |
| NTC R2             | 10kΩ   | NTC-Thermistor (Nennwert bei 25°C)       |
| B-Koeffizient      | 3950   | Typisch für 10kΩ NTC                     |

---

## Gesamtverdrahtung (Schematisch)

```
                    ┌────────────────────────────────────────┐
                    │              ESP32 DevKit              │
                    │                                         │
5V ─────────────────┤ 5V                    GND ─────────────┤─── GND (alle)
                    │                                         │
Nextion TX ─────────┤ GPIO16 (RX2)   GPIO17 (TX2) ───────────┤─── Nextion RX
                    │                                         │
RS485 RO ───────────┤ GPIO26 (RX1)   GPIO25 (TX1) ───────────┤─── RS485 DI
                    │                                         │
                    │               GPIO27 ─────────────────┤─── RS485 DE+RE
                    │                                         │
DS18B20 Data ───────┤ GPIO4                                   │
                    │                                         │
NTC Mittelabgriff ──┤ GPIO34 (ADC)                            │
                    │                                         │
3.3V ───────────────┤ 3V3                                     │
                    └────────────────────────────────────────┘
```

---

## Spannungsversorgung

| Komponente          | Spannung | Strom (max) | Hinweis                           |
|---------------------|----------|-------------|-----------------------------------|
| ESP32               | 5V (USB) / 3.3V | 240mA | USB oder 3.3V-Regler          |
| Nextion NX8048P070  | 5V       | ~800mA      | Eigenes 5V Netzteil empfohlen     |
| RS485 Modul         | 3.3V     | ~30mA       | Vom ESP32 3.3V-Pin versorgt       |
| DS18B20             | 3.3V     | ~1.5mA      | Vom ESP32 3.3V-Pin versorgt       |
| **Gesamt**          | 5V       | **~1A**     | 5V/2A Netzteil empfehlenswert     |

> **Empfehlung:** ESP32 und Nextion Display über ein gemeinsames **5V/2A Netzteil** versorgen. Das Nextion-Display hat seinen eigenen internen 3.3V-Regler.

---

## Kabelempfehlungen

| Verbindung              | Kabeltyp             | Länge (max) | Hinweis                       |
|-------------------------|----------------------|-------------|-------------------------------|
| ESP32 ↔ Nextion         | 4-adriges Kabel      | 1m          | Kurze Verbindung genügt       |
| ESP32 ↔ RS485 Modul     | 4-adriges Kabel      | 30cm        | Im selben Gehäuse             |
| RS485 Bus               | Verdrilltes Paar (TP) | 1000m      | Abschlusswiderstände 120Ω an den Enden |
| DS18B20                 | 3-adriges Kabel      | 10m         | Pull-Up am Sensor-Ende        |

---

## Sicherheitshinweise

1. **RS485-Bus-Abschluss:** An beiden Enden des RS485-Busses je einen **120Ω-Widerstand** zwischen A(+) und B(-) einbauen.
2. **Potentialtrennung:** Bei langen RS485-Leitungen oder Unterschieden in der Erdung ein **galvanisch getrenntes RS485-Modul** verwenden (z.B. ADM2483).
3. **ESD-Schutz:** RS485-Leitungen sind anfällig für Überspannungen – TVS-Dioden an A und B empfehlenswert.
4. **Gehäuse:** Alle Komponenten in einem geeigneten Schaltschrank oder Aufputzgehäuse (min. IP20) installieren.
