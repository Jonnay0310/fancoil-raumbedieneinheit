#pragma once

/**
 * @file fancoil_controller.h
 * @brief Fan-Coil Steuerungslogik für die Raumbedieneinheit
 *
 * Enthält die gesamte Regellogik:
 * - Betriebsmodus-Verwaltung (Heizen/Kühlen/Auto/Aus)
 * - Lüfterstufen-Berechnung
 * - Ventilsteuerung
 * - Frostschutz-Funktion
 * - Eco-Modus
 * - Temperatursensor-Verwaltung (DS18B20 oder NTC)
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "register_map.h"

// ============================================================
// KLASSE FancoilController
// ============================================================

class FancoilController {
public:
    /**
     * @brief Initialisiert den Controller und den Temperatursensor
     * @param tempSource Temperaturquelle (TEMP_SOURCE_LOCAL oder TEMP_SOURCE_MODBUS)
     */
    void begin(uint8_t tempSource = TEMP_SOURCE_LOCAL);

    /**
     * @brief Führt einen Steuerungszyklus aus (regelmäßig aufrufen)
     */
    void update();

    /**
     * @brief Liest den lokalen Temperatursensor
     * @return Temperatur ×10 (z.B. 235 = 23.5°C), oder INT16_MIN bei Fehler
     */
    int16_t readLocalTemperature();

    // ---- Setter ----

    /** Setzt die Solltemperatur (×10) */
    void setSetpoint(int16_t setpointX10);

    /** Setzt den Betriebsmodus */
    void setMode(uint8_t mode);

    /** Setzt die gewünschte Lüfterstufe */
    void setFanSpeed(uint8_t fanSpeed);

    /** Aktiviert/deaktiviert den Eco-Modus */
    void setEcoMode(bool enabled);

    /** Setzt die Temperaturquelle */
    void setTempSource(uint8_t source);

    /** Setzt die Ist-Temperatur (wenn Quelle = Modbus) */
    void setActualTemp(int16_t tempX10);

    /** Setzt die Eco-Absenkung (×10) */
    void setEcoOffset(int16_t offsetX10);

    /** Setzt die Frostschutz-Temperatur (×10) */
    void setFrostTemp(int16_t frostTempX10);

    // ---- Getter ----

    /** Gibt die aktuelle Ist-Temperatur zurück (×10) */
    int16_t getActualTemp() const;

    /** Gibt die aktuelle Solltemperatur zurück (×10, inkl. Eco-Absenkung) */
    int16_t getEffectiveSetpoint() const;

    /** Gibt die rohe Solltemperatur zurück (×10, ohne Eco) */
    int16_t getSetpoint() const;

    /** Gibt den aktuellen Betriebsmodus zurück */
    uint8_t getMode() const;

    /** Gibt den effektiven Modus zurück (berücksichtigt Frostschutz, Aus) */
    uint8_t getEffectiveMode() const;

    /** Gibt die aktive Lüfterstufe zurück */
    uint8_t getActiveFanSpeed() const;

    /** Gibt die Ventilstellung zurück (0–100%) */
    uint8_t getValvePosition() const;

    /** Gibt zurück, ob Eco-Modus aktiv ist */
    bool isEcoActive() const;

    /** Gibt zurück, ob Frostschutz aktiv ist */
    bool isFrostActive() const;

    /** Gibt den Fehlerstatus zurück (Bitfeld) */
    uint16_t getErrorStatus() const;

    /** Setzt ein Fehlerbit */
    void setError(uint16_t errorBit);

    /** Löscht ein Fehlerbit */
    void clearError(uint16_t errorBit);

private:
    // Temperatursensor
    OneWire         _oneWire;
    DallasTemperature _ds18b20;

    // Aktueller Zustand
    int16_t  _actualTemp      = 0;      // Ist-Temperatur ×10
    int16_t  _setpoint        = SETPOINT_DEFAULT;  // Solltemperatur ×10
    int16_t  _ecoOffset       = ECO_OFFSET_DEFAULT; // Eco-Absenkung ×10
    int16_t  _frostTemp       = FROST_TEMP_DEFAULT; // Frostschutz-Temperatur ×10
    uint8_t  _mode            = MODE_OFF;
    uint8_t  _fanSpeed        = FAN_AUTO;
    uint8_t  _activeFanSpeed  = FAN_AUTO;
    uint8_t  _valvePosition   = 0;
    uint8_t  _tempSource      = TEMP_SOURCE_LOCAL;
    bool     _ecoActive       = false;
    bool     _frostActive     = false;
    uint16_t _errorStatus     = 0;

    // Letzter Sensor-Lesezeitpunkt
    uint32_t _lastSensorRead  = 0;

    // Regellogik-Hilfsfunktionen
    void runControlLogic();
    void calculateValvePosition();
    uint8_t calculateFanSpeed();
    bool checkFrostProtection();
    int16_t getNtcTemperature();
};
