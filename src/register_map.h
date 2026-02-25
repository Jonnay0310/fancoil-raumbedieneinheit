#pragma once

/**
 * @file register_map.h
 * @brief Modbus Register-Map für Fan-Coil Raumbedieneinheit
 *
 * Definiert alle Modbus Holding- und Input-Register Adressen
 * sowie Konstanten für Wertebereiche.
 */

// ============================================================
// HOLDING REGISTER (R/W) – Einstellungen
// Adressbereich: 0x0000 – 0x0007
// ============================================================

/** Solltemperatur (×10, z.B. 220 = 22.0°C), Bereich: 180–280 */
#define REG_SETPOINT         0x0000

/** Lüfterstufe (0=Auto, 1=Stufe 1, 2=Stufe 2, 3=Stufe 3) */
#define REG_FAN_SPEED        0x0001

/** Betriebsmodus (0=Aus, 1=Heizen, 2=Kühlen, 3=Auto) */
#define REG_MODE             0x0002

/** Eco-Modus (0=Aus, 1=Ein) */
#define REG_ECO_MODE         0x0003

/** Temperaturquelle (0=Lokal, 1=Modbus) */
#define REG_TEMP_SOURCE      0x0004

/** Slave-Adresse (1–247) */
#define REG_SLAVE_ADDR       0x0005

/** Eco-Absenkung (×10, z.B. 20 = 2.0°C), Bereich: 10–50 */
#define REG_ECO_OFFSET       0x0006

/** Frostschutz-Temperatur (×10, z.B. 50 = 5.0°C), Bereich: 30–100 */
#define REG_FROST_TEMP       0x0007

// ============================================================
// INPUT REGISTER (Read Only) – Statuswerte
// Adressbereich: 0x0010 – 0x0015
// ============================================================

/** Ist-Temperatur (×10, z.B. 235 = 23.5°C), Bereich: -200–500 */
#define REG_ACTUAL_TEMP      0x0010

/** Ventilstellung (0–100%) */
#define REG_VALVE_POSITION   0x0011

/** Aktive Lüfterstufe (0–3) */
#define REG_ACTIVE_FAN       0x0012

/** Fehlerstatus (Bitfeld, 0x0000 = kein Fehler) */
#define REG_ERROR_STATUS     0x0013

/** Aktueller effektiver Modus (0–3) */
#define REG_ACTIVE_MODE      0x0014

/** Frostschutz aktiv (0=Nein, 1=Ja) */
#define REG_FROST_ACTIVE     0x0015

// ============================================================
// REGISTER ANZAHL
// ============================================================

/** Anzahl der Holding Register */
#define HOLDING_REG_COUNT    8

/** Startadresse der Input Register */
#define INPUT_REG_START      0x0010

/** Anzahl der Input Register */
#define INPUT_REG_COUNT      6

// ============================================================
// FEHLERSTATUS BITS (REG_ERROR_STATUS Bitfeld)
// ============================================================

/** Bit 0: Temperatursensor-Fehler */
#define ERR_TEMP_SENSOR      (1 << 0)

/** Bit 1: Modbus-Kommunikationsfehler */
#define ERR_MODBUS_COMM      (1 << 1)

/** Bit 2: Display-Kommunikationsfehler */
#define ERR_DISPLAY_COMM     (1 << 2)

/** Bit 3: NVS/EEPROM-Fehler */
#define ERR_NVS              (1 << 3)
