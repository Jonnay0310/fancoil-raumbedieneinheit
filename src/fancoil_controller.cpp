/**
 * @file fancoil_controller.cpp
 * @brief Implementierung der Fan-Coil Steuerungslogik
 */

#include "fancoil_controller.h"
#include <math.h>

// ============================================================
// INITIALISIERUNG
// ============================================================

void FancoilController::begin(uint8_t tempSource) {
    _tempSource = tempSource;

    if (_tempSource == TEMP_SOURCE_LOCAL) {
        // DS18B20 OneWire Sensor initialisieren
        _oneWire.begin(ONEWIRE_PIN);
        _ds18b20.setOneWire(&_oneWire);
        _ds18b20.begin();

        uint8_t deviceCount = _ds18b20.getDeviceCount();
        if (deviceCount == 0) {
            DEBUG_PRINTLN("[Controller] Kein DS18B20 Sensor gefunden, verwende NTC");
            setError(ERR_TEMP_SENSOR);
        } else {
            DEBUG_PRINTF("[Controller] DS18B20 Sensoren gefunden: %d\n", deviceCount);
            _ds18b20.setResolution(12);  // 12-Bit Auflösung (0.0625°C)
        }
    }

    DEBUG_PRINTLN("[Controller] Fan-Coil Controller initialisiert");
}

// ============================================================
// UPDATE – regelmäßig aufrufen
// ============================================================

void FancoilController::update() {
    uint32_t now = millis();

    // Lokalen Sensor regelmäßig lesen
    if (_tempSource == TEMP_SOURCE_LOCAL &&
        (now - _lastSensorRead) >= SENSOR_READ_INTERVAL_MS) {
        _lastSensorRead = now;
        int16_t temp = readLocalTemperature();
        if (temp != INT16_MIN) {
            _actualTemp = temp;
            clearError(ERR_TEMP_SENSOR);
        } else {
            setError(ERR_TEMP_SENSOR);
        }
    }

    // Regellogik ausführen
    runControlLogic();
}

// ============================================================
// TEMPERATURSENSOR LESEN
// ============================================================

int16_t FancoilController::readLocalTemperature() {
    // Zuerst DS18B20 versuchen
    if (_ds18b20.getDeviceCount() > 0) {
        _ds18b20.requestTemperatures();
        float tempC = _ds18b20.getTempCByIndex(0);

        if (tempC == DEVICE_DISCONNECTED_C) {
            DEBUG_PRINTLN("[Controller] DS18B20 getrennt, versuche NTC");
            // Fallback auf NTC
            return getNtcTemperature();
        }

        // Temperatur als ×10 zurückgeben
        return (int16_t)(tempC * 10.0f);
    } else {
        // NTC verwenden
        return getNtcTemperature();
    }
}

/**
 * @brief Liest die Temperatur vom NTC-Widerstand
 * Verwendet die Steinhart-Hart-Gleichung (vereinfacht)
 */
int16_t FancoilController::getNtcTemperature() {
    // ADC-Wert lesen (12-Bit ESP32: 0–4095)
    int adcValue = analogRead(NTC_PIN);
    if (adcValue <= 0) return INT16_MIN;

    // Widerstand berechnen
    float resistance = NTC_SERIES_RESISTOR * ((4095.0f / adcValue) - 1.0f);

    // Steinhart-Hart (vereinfachte B-Formel)
    float steinhart = resistance / NTC_NOMINAL_RES;
    steinhart = log(steinhart);
    steinhart /= NTC_BCOEFFICIENT;
    steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    float tempC = steinhart - 273.15f;

    if (tempC < -50.0f || tempC > 100.0f) {
        DEBUG_PRINTLN("[Controller] NTC Temperatur ausserhalb Bereich");
        return INT16_MIN;
    }

    return (int16_t)(tempC * 10.0f);
}

// ============================================================
// REGELLOGIK
// ============================================================

void FancoilController::runControlLogic() {
    // Frostschutz prüfen (hat höchste Priorität)
    _frostActive = checkFrostProtection();

    // Effektiven Modus bestimmen
    uint8_t effectiveMode = getEffectiveMode();

    // Ventilstellung berechnen
    calculateValvePosition();

    // Lüfterstufe berechnen
    _activeFanSpeed = calculateFanSpeed();
}

/**
 * @brief Überprüft und aktiviert den Frostschutz
 * Frostschutz ist aktiv, wenn:
 * - Modus = Aus UND Ist-Temperatur < Frostschutz-Temperatur
 */
bool FancoilController::checkFrostProtection() {
    if (_actualTemp < _frostTemp) {
        if (!_frostActive) {
            DEBUG_PRINTF("[Controller] Frostschutz aktiviert! Temp=%d Grenze=%d\n",
                         _actualTemp, _frostTemp);
        }
        return true;
    }
    return false;
}

/**
 * @brief Berechnet die Ventilstellung basierend auf Ist-/Solltemperatur
 * Einfacher P-Regler:
 * - Heizen: Ventil auf wenn Ist < Soll
 * - Kühlen: Ventil auf wenn Ist > Soll
 */
void FancoilController::calculateValvePosition() {
    uint8_t effectiveMode = getEffectiveMode();

    if (effectiveMode == MODE_OFF && !_frostActive) {
        _valvePosition = 0;
        return;
    }

    int16_t effectiveSP = getEffectiveSetpoint();
    int16_t error = _actualTemp - effectiveSP;  // Positiv = zu warm

    // P-Regler mit Totband ±5 (±0.5°C)
    const int16_t DEADBAND = 5;

    if (_frostActive || effectiveMode == MODE_HEAT) {
        // Heizen: Ventil auf wenn zu kalt
        if (error < -DEADBAND) {
            // Proportional: max 100% bei -5°C Differenz (50 in ×10)
            int16_t pVal = (int16_t)((-error - DEADBAND) * 100 / 50);
            _valvePosition = (uint8_t)constrain(pVal, 10, 100);
        } else {
            _valvePosition = 0;
        }
    } else if (effectiveMode == MODE_COOL) {
        // Kühlen: Ventil auf wenn zu warm
        if (error > DEADBAND) {
            int16_t pVal = (int16_t)((error - DEADBAND) * 100 / 50);
            _valvePosition = (uint8_t)constrain(pVal, 10, 100);
        } else {
            _valvePosition = 0;
        }
    } else if (effectiveMode == MODE_AUTO) {
        // Auto: heizen oder kühlen je nach Differenz
        if (error < -DEADBAND) {
            int16_t pVal = (int16_t)((-error - DEADBAND) * 100 / 50);
            _valvePosition = (uint8_t)constrain(pVal, 10, 100);
        } else if (error > DEADBAND) {
            int16_t pVal = (int16_t)((error - DEADBAND) * 100 / 50);
            _valvePosition = (uint8_t)constrain(pVal, 10, 100);
        } else {
            _valvePosition = 0;
        }
    }
}

/**
 * @brief Berechnet die aktive Lüfterstufe
 * Bei FAN_AUTO: Stufe abhängig von Ventilstellung
 */
uint8_t FancoilController::calculateFanSpeed() {
    uint8_t effectiveMode = getEffectiveMode();

    // Lüfter aus wenn Modus = Aus und kein Frostschutz und Ventil = 0
    if (effectiveMode == MODE_OFF && !_frostActive && _valvePosition == 0) {
        return FAN_AUTO;  // Lüfter aus
    }

    if (_fanSpeed != FAN_AUTO) {
        // Manuelle Stufe verwenden
        return _fanSpeed;
    }

    // Auto-Berechnung basierend auf Ventilstellung
    if (_valvePosition == 0) {
        return FAN_AUTO;  // Lüfter aus
    } else if (_valvePosition < 33) {
        return FAN_SPEED_1;
    } else if (_valvePosition < 66) {
        return FAN_SPEED_2;
    } else {
        return FAN_SPEED_3;
    }
}

// ============================================================
// SETTER
// ============================================================

void FancoilController::setSetpoint(int16_t setpointX10) {
    _setpoint = constrain(setpointX10, SETPOINT_MIN, SETPOINT_MAX);
    DEBUG_PRINTF("[Controller] Solltemperatur: %d (×10)\n", _setpoint);
}

void FancoilController::setMode(uint8_t mode) {
    _mode = constrain(mode, 0, 3);
    DEBUG_PRINTF("[Controller] Modus: %d\n", _mode);
}

void FancoilController::setFanSpeed(uint8_t fanSpeed) {
    _fanSpeed = constrain(fanSpeed, 0, 3);
    DEBUG_PRINTF("[Controller] Lüfterstufe: %d\n", _fanSpeed);
}

void FancoilController::setEcoMode(bool enabled) {
    _ecoActive = enabled;
    DEBUG_PRINTF("[Controller] Eco-Modus: %s\n", enabled ? "AN" : "AUS");
}

void FancoilController::setTempSource(uint8_t source) {
    _tempSource = constrain(source, 0, 1);
}

void FancoilController::setActualTemp(int16_t tempX10) {
    if (_tempSource == TEMP_SOURCE_MODBUS) {
        _actualTemp = tempX10;
    }
}

void FancoilController::setEcoOffset(int16_t offsetX10) {
    _ecoOffset = constrain(offsetX10, 10, 50);
}

void FancoilController::setFrostTemp(int16_t frostTempX10) {
    _frostTemp = constrain(frostTempX10, 30, 100);
}

// ============================================================
// GETTER
// ============================================================

int16_t FancoilController::getActualTemp() const {
    return _actualTemp;
}

int16_t FancoilController::getEffectiveSetpoint() const {
    int16_t sp = _setpoint;
    if (_ecoActive) {
        sp -= _ecoOffset;
    }
    return sp;
}

int16_t FancoilController::getSetpoint() const {
    return _setpoint;
}

uint8_t FancoilController::getMode() const {
    return _mode;
}

uint8_t FancoilController::getEffectiveMode() const {
    // Frostschutz übersteuert alles – immer Heizen
    if (_frostActive) {
        return MODE_HEAT;
    }
    return _mode;
}

uint8_t FancoilController::getActiveFanSpeed() const {
    return _activeFanSpeed;
}

uint8_t FancoilController::getValvePosition() const {
    return _valvePosition;
}

bool FancoilController::isEcoActive() const {
    return _ecoActive;
}

bool FancoilController::isFrostActive() const {
    return _frostActive;
}

uint16_t FancoilController::getErrorStatus() const {
    return _errorStatus;
}

void FancoilController::setError(uint16_t errorBit) {
    _errorStatus |= errorBit;
}

void FancoilController::clearError(uint16_t errorBit) {
    _errorStatus &= ~errorBit;
}
