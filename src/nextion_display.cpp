/**
 * @file nextion_display.cpp
 * @brief Implementierung der Nextion Display Kommunikation
 *
 * Das Nextion-Protokoll sendet Befehle als ASCII-Strings, abgeschlossen
 * mit drei 0xFF-Bytes. Ereignisse vom Display werden als Binärdaten empfangen.
 */

#include "nextion_display.h"

// ============================================================
// INITIALISIERUNG
// ============================================================

void NextionDisplay::begin() {
    // UART für Nextion initialisieren
    NEXTION_SERIAL.begin(NEXTION_BAUDRATE, SERIAL_8N1,
                         NEXTION_RX_PIN, NEXTION_TX_PIN);
    delay(100);

    // Startseite anzeigen
    switchPage(PAGE_MAIN);

    DEBUG_PRINTLN("[Nextion] Display initialisiert");
}

// ============================================================
// UPDATE – regelmäßig aufrufen
// ============================================================

void NextionDisplay::update() {
    // Eingehende Bytes lesen
    while (NEXTION_SERIAL.available()) {
        uint8_t byte = NEXTION_SERIAL.read();
        if (_rxPos < RX_BUF_SIZE) {
            _rxBuf[_rxPos++] = byte;
        }

        // Auf drei aufeinanderfolgende 0xFF warten (Nachrichtenende)
        if (_rxPos >= 3 &&
            _rxBuf[_rxPos - 1] == 0xFF &&
            _rxBuf[_rxPos - 2] == 0xFF &&
            _rxBuf[_rxPos - 3] == 0xFF) {
            // Nachricht vollständig – verarbeiten
            processReceivedData(_rxBuf, _rxPos);
            _rxPos = 0;
        }
    }
}

// ============================================================
// HAUPTBILDSCHIRM AKTUALISIEREN
// ============================================================

void NextionDisplay::updateMainScreen(int16_t actualTemp, int16_t setpoint,
                                       uint8_t mode, uint8_t fanSpeed,
                                       uint8_t valvePos, bool ecoActive,
                                       bool frostActive, uint16_t errorStatus) {
    char buf[32];

    // Ist-Temperatur
    formatTemperature(actualTemp, buf, sizeof(buf));
    setTextValue(NX_ACTUAL_TEMP, buf);

    // Solltemperatur
    formatTemperature(setpoint, buf, sizeof(buf));
    setTextValue(NX_SETPOINT, buf);

    // Betriebsmodus
    setTextValue(NX_MODE_DISPLAY, getModeString(mode));

    // Lüfterstufe
    setTextValue(NX_FAN_DISPLAY, getFanString(fanSpeed));

    // Ventilstellung
    snprintf(buf, sizeof(buf), "%d%%", valvePos);
    setTextValue(NX_VALVE_DISPLAY, buf);

    // Eco-Modus Status
    setTextValue(NX_ECO_DISPLAY, ecoActive ? "ECO" : "");

    // Frostschutz Status
    setTextValue(NX_FROST_DISPLAY, frostActive ? "FROST" : "");

    // Fehlerstatus
    if (errorStatus != 0) {
        snprintf(buf, sizeof(buf), "ERR:0x%04X", errorStatus);
        setTextValue(NX_ERROR_DISPLAY, buf);
    } else {
        setTextValue(NX_ERROR_DISPLAY, "");
    }
}

// ============================================================
// EINSTELLUNGSBILDSCHIRM AKTUALISIEREN
// ============================================================

void NextionDisplay::updateSettingsScreen(int16_t setpoint, uint8_t mode,
                                           uint8_t fanSpeed, bool ecoActive) {
    // Solltemperatur als Zahlenwert (×10)
    setNumericValue(NX_SET_SETPOINT, setpoint);

    // Modus
    setNumericValue(NX_SET_MODE, mode);

    // Lüfterstufe
    setNumericValue(NX_SET_FAN, fanSpeed);

    // Eco-Modus
    setNumericValue(NX_SET_ECO, ecoActive ? 1 : 0);
}

// ============================================================
// PRIMITIVE SCHREIBFUNKTIONEN
// ============================================================

void NextionDisplay::setTextValue(const char* objectName, const char* value) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s.txt=\"%s\"", objectName, value);
    sendCommand(cmd);
}

void NextionDisplay::setNumericValue(const char* objectName, int32_t value) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s.val=%ld", objectName, (long)value);
    sendCommand(cmd);
}

void NextionDisplay::switchPage(uint8_t pageId) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "page %d", pageId);
    sendCommand(cmd);
    _currentPage = pageId;
    DEBUG_PRINTF("[Nextion] Seite gewechselt zu %d\n", pageId);
}

// ============================================================
// CALLBACK REGISTRIERUNG
// ============================================================

void NextionDisplay::onSetpointChanged(void (*cb)(int16_t)) {
    _cbSetpoint = cb;
}

void NextionDisplay::onModeChanged(void (*cb)(uint8_t)) {
    _cbMode = cb;
}

void NextionDisplay::onFanSpeedChanged(void (*cb)(uint8_t)) {
    _cbFan = cb;
}

void NextionDisplay::onEcoModeChanged(void (*cb)(bool)) {
    _cbEco = cb;
}

void NextionDisplay::onScheduleChanged(void (*cb)(uint8_t, uint8_t, uint8_t,
                                                   uint8_t, uint8_t, uint8_t,
                                                   uint8_t)) {
    _cbSchedule = cb;
}

void NextionDisplay::onSystemSettingChanged(void (*cb)(uint8_t, int16_t)) {
    _cbSystem = cb;
}

// ============================================================
// PRIVATE HILFSFUNKTIONEN
// ============================================================

void NextionDisplay::sendTerminator() {
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
}

void NextionDisplay::sendCommand(const char* cmd) {
    NEXTION_SERIAL.print(cmd);
    sendTerminator();
}

/**
 * @brief Verarbeitet empfangene Nextion-Ereignisse
 *
 * Nextion Ereignis-Codes:
 *  0x65 – Touch-Ereignis (Seite, Komponenten-ID, Ereignistyp)
 *  0x66 – Seitenkennung
 *  0x71 – Numerischer Wert (4 Bytes, Little-Endian)
 *  0x70 – Textwert
 */
void NextionDisplay::processReceivedData(uint8_t* data, uint8_t len) {
    if (len < 4) return;  // Mindestlänge: Code + 3x 0xFF

    uint8_t eventCode = data[0];

    switch (eventCode) {
        // Touch-Ereignis: [0x65][Seite][Komponente][Ereignis][0xFF][0xFF][0xFF]
        case 0x65: {
            if (len < 7) break;
            uint8_t page      = data[1];
            uint8_t component = data[2];
            uint8_t event     = data[3];  // 0=loslassen, 1=drücken

            if (event != 1) break;  // Nur bei Drücken reagieren

            DEBUG_PRINTF("[Nextion] Touch: Seite=%d Komp=%d\n", page, component);

            // Seite 1 – Einstellungen: Solltemperatur +/-
            if (page == PAGE_SETTINGS) {
                // Komponente 1 = Erhöhen (+0.5°C = +5 in ×10)
                if (component == 1 && _cbSetpoint) {
                    // Wird vom Hauptprogramm mit aktuellem Wert + 5 aufgerufen
                    // Hier senden wir Ereignis mit speziellen Wert-Code
                    _cbSetpoint(9999);  // Signal: erhöhen
                }
                // Komponente 2 = Verringern
                else if (component == 2 && _cbSetpoint) {
                    _cbSetpoint(-9999);  // Signal: verringern
                }
                // Komponente 3–6: Modus-Schaltflächen (0=Aus, 1=Heizen, 2=Kühlen, 3=Auto)
                else if (component >= 3 && component <= 6 && _cbMode) {
                    _cbMode(component - 3);
                }
                // Komponente 7–10: Lüfterstufen-Schaltflächen
                else if (component >= 7 && component <= 10 && _cbFan) {
                    _cbFan(component - 7);
                }
                // Komponente 11: Eco-Modus Toggle
                else if (component == 11 && _cbEco) {
                    // Toggle wird vom Hauptprogramm verwaltet
                    _cbEco(true);  // Signal: Toggle
                }
            }
            break;
        }

        // Numerischer Rückgabewert: [0x71][Val0][Val1][Val2][Val3][0xFF][0xFF][0xFF]
        case 0x71: {
            if (len < 8) break;
            // Little-Endian 32-Bit Wert
            int32_t val = (int32_t)(data[1] | (data[2] << 8) |
                                    (data[3] << 16) | (data[4] << 24));
            DEBUG_PRINTF("[Nextion] Numerischer Wert: %ld\n", (long)val);
            break;
        }

        default:
            DEBUG_PRINTF("[Nextion] Unbekannter Ereigniscode: 0x%02X\n", eventCode);
            break;
    }
}

void NextionDisplay::formatTemperature(int16_t tempX10, char* buf, uint8_t bufLen) {
    // Vorzeichen ermitteln
    bool negative = (tempX10 < 0);
    int16_t absVal = negative ? -tempX10 : tempX10;
    int16_t intPart  = absVal / 10;
    int16_t fracPart = absVal % 10;
    snprintf(buf, bufLen, "%s%d.%d°C", negative ? "-" : "", intPart, fracPart);
}

const char* NextionDisplay::getModeString(uint8_t mode) {
    switch (mode) {
        case MODE_OFF:   return "AUS";
        case MODE_HEAT:  return "HEIZEN";
        case MODE_COOL:  return "KÜHLEN";
        case MODE_AUTO:  return "AUTO";
        default:         return "???";
    }
}

const char* NextionDisplay::getFanString(uint8_t fanSpeed) {
    switch (fanSpeed) {
        case FAN_AUTO:     return "AUTO";
        case FAN_SPEED_1:  return "STUFE 1";
        case FAN_SPEED_2:  return "STUFE 2";
        case FAN_SPEED_3:  return "STUFE 3";
        default:           return "???";
    }
}
