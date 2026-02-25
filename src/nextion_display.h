#pragma once

/**
 * @file nextion_display.h
 * @brief Nextion Display Kommunikation für Fan-Coil Raumbedieneinheit
 *
 * Implementiert die Kommunikation mit dem Nextion NX8048P070-011C Display
 * über UART. Steuert alle Seiten und Elemente des HMI.
 */

#include <Arduino.h>
#include "config.h"

// ============================================================
// NEXTION SEITEN-IDs
// ============================================================

#define PAGE_MAIN            0   // Hauptbildschirm
#define PAGE_SETTINGS        1   // Einstellungen
#define PAGE_SCHEDULE        2   // Zeitprogramm
#define PAGE_SYSTEM          3   // Systemeinstellungen

// ============================================================
// NEXTION OBJEKT-NAMEN (Hauptbildschirm – Seite 0)
// ============================================================

// Ist-Temperatur Anzeige (Textfeld)
#define NX_ACTUAL_TEMP       "t_actual_temp"
// Solltemperatur Anzeige (Textfeld)
#define NX_SETPOINT          "t_setpoint"
// Modus Anzeige (Textfeld)
#define NX_MODE_DISPLAY      "t_mode"
// Lüfterstufe Anzeige (Textfeld)
#define NX_FAN_DISPLAY       "t_fan"
// Ventilstellung Anzeige (Textfeld)
#define NX_VALVE_DISPLAY     "t_valve"
// Eco-Modus Anzeige (Textfeld)
#define NX_ECO_DISPLAY       "t_eco"
// Frostschutz Anzeige (Textfeld)
#define NX_FROST_DISPLAY     "t_frost"
// Fehleranzeige (Textfeld)
#define NX_ERROR_DISPLAY     "t_error"

// ============================================================
// NEXTION OBJEKT-NAMEN (Einstellungen – Seite 1)
// ============================================================

// Solltemperatur Eingabe (Zahlenfeld)
#define NX_SET_SETPOINT      "n_setpoint"
// Modus Auswahl (Dropdown/Schaltflächen)
#define NX_SET_MODE          "n_mode"
// Lüfterstufe Auswahl
#define NX_SET_FAN           "n_fan"
// Eco-Modus Schalter
#define NX_SET_ECO           "n_eco"

// ============================================================
// NEXTION OBJEKT-NAMEN (Zeitprogramm – Seite 2)
// ============================================================

// Wochentag Auswahl (0=Montag, ..., 6=Sonntag)
#define NX_SCHED_DAY         "n_day"
// Zeitfenster-Nummer (0–3)
#define NX_SCHED_SLOT        "n_slot"
// Startzeit Stunde
#define NX_SCHED_START_H     "n_start_h"
// Startzeit Minute
#define NX_SCHED_START_M     "n_start_m"
// Endzeit Stunde
#define NX_SCHED_END_H       "n_end_h"
// Endzeit Minute
#define NX_SCHED_END_M       "n_end_m"
// Modus für dieses Zeitfenster (0=Komfort, 1=Eco, 2=Aus)
#define NX_SCHED_MODE        "n_sched_mode"

// ============================================================
// NEXTION OBJEKT-NAMEN (System – Seite 3)
// ============================================================

// Modbus Slave-Adresse
#define NX_SYS_SLAVE_ADDR    "n_slave_addr"
// Temperaturquelle (0=Lokal, 1=Modbus)
#define NX_SYS_TEMP_SRC      "n_temp_src"
// Eco-Absenkung
#define NX_SYS_ECO_OFFSET    "n_eco_offset"
// Frostschutz-Temperatur
#define NX_SYS_FROST_TEMP    "n_frost_temp"

// ============================================================
// KLASSE NextionDisplay
// ============================================================

class NextionDisplay {
public:
    /**
     * @brief Initialisiert die Nextion-Kommunikation
     */
    void begin();

    /**
     * @brief Liest eingehende Nextion-Daten und verarbeitet Ereignisse
     * Muss regelmäßig im Loop aufgerufen werden.
     */
    void update();

    // ---- Schreibfunktionen ----

    /**
     * @brief Setzt einen Textwert auf dem Display
     * @param objectName Name des Nextion-Textfelds
     * @param value Anzuzeigender Text
     */
    void setTextValue(const char* objectName, const char* value);

    /**
     * @brief Setzt einen Zahlenwert auf dem Display
     * @param objectName Name des Nextion-Zahlfelds
     * @param value Anzuzeigender Wert
     */
    void setNumericValue(const char* objectName, int32_t value);

    /**
     * @brief Wechselt zu einer bestimmten Seite
     * @param pageId Seiten-ID (0–3)
     */
    void switchPage(uint8_t pageId);

    /**
     * @brief Aktualisiert den Hauptbildschirm mit allen Statuswerten
     * @param actualTemp     Ist-Temperatur (×10)
     * @param setpoint       Solltemperatur (×10)
     * @param mode           Aktueller Modus (0–3)
     * @param fanSpeed       Lüfterstufe (0–3)
     * @param valvePos       Ventilstellung (0–100%)
     * @param ecoActive      Eco-Modus aktiv
     * @param frostActive    Frostschutz aktiv
     * @param errorStatus    Fehlerstatus (Bitfeld)
     */
    void updateMainScreen(int16_t actualTemp, int16_t setpoint, uint8_t mode,
                          uint8_t fanSpeed, uint8_t valvePos,
                          bool ecoActive, bool frostActive, uint16_t errorStatus);

    /**
     * @brief Aktualisiert den Einstellungsbildschirm
     * @param setpoint   Solltemperatur (×10)
     * @param mode       Modus (0–3)
     * @param fanSpeed   Lüfterstufe (0–3)
     * @param ecoActive  Eco-Modus aktiv
     */
    void updateSettingsScreen(int16_t setpoint, uint8_t mode,
                               uint8_t fanSpeed, bool ecoActive);

    // ---- Callback-Setter ----

    /**
     * @brief Registriert Callback für Solltemperatur-Änderung vom Display
     * @param cb Callback-Funktion: void(int16_t newSetpoint)
     */
    void onSetpointChanged(void (*cb)(int16_t));

    /**
     * @brief Registriert Callback für Modus-Änderung vom Display
     * @param cb Callback-Funktion: void(uint8_t newMode)
     */
    void onModeChanged(void (*cb)(uint8_t));

    /**
     * @brief Registriert Callback für Lüfterstufen-Änderung vom Display
     * @param cb Callback-Funktion: void(uint8_t newFanSpeed)
     */
    void onFanSpeedChanged(void (*cb)(uint8_t));

    /**
     * @brief Registriert Callback für Eco-Modus-Änderung vom Display
     * @param cb Callback-Funktion: void(bool ecoOn)
     */
    void onEcoModeChanged(void (*cb)(bool));

    /**
     * @brief Registriert Callback für Zeitprogramm-Änderungen
     * @param cb Callback-Funktion: void(uint8_t day, uint8_t slot, ...)
     */
    void onScheduleChanged(void (*cb)(uint8_t, uint8_t, uint8_t, uint8_t,
                                      uint8_t, uint8_t, uint8_t));

    /**
     * @brief Registriert Callback für Systemeinstellungs-Änderungen
     * @param cb Callback-Funktion: void(uint8_t key, int16_t value)
     */
    void onSystemSettingChanged(void (*cb)(uint8_t, int16_t));

private:
    // Nextion Terminator: drei 0xFF Bytes
    void sendTerminator();

    // Rohdaten an Display senden
    void sendCommand(const char* cmd);

    // Eingehende Daten auswerten
    void processReceivedData(uint8_t* data, uint8_t len);

    // Temperatur als formatierten String zurückgeben (z.B. "22.5°C")
    void formatTemperature(int16_t tempX10, char* buf, uint8_t bufLen);

    // Modusnamen zurückgeben
    const char* getModeString(uint8_t mode);

    // Lüfterstufen-String zurückgeben
    const char* getFanString(uint8_t fanSpeed);

    // Empfangspuffer
    static const uint8_t RX_BUF_SIZE = 64;
    uint8_t _rxBuf[RX_BUF_SIZE];
    uint8_t _rxPos = 0;

    // Callbacks
    void (*_cbSetpoint)(int16_t)       = nullptr;
    void (*_cbMode)(uint8_t)           = nullptr;
    void (*_cbFan)(uint8_t)            = nullptr;
    void (*_cbEco)(bool)               = nullptr;
    void (*_cbSchedule)(uint8_t, uint8_t, uint8_t, uint8_t,
                        uint8_t, uint8_t, uint8_t) = nullptr;
    void (*_cbSystem)(uint8_t, int16_t) = nullptr;

    // Aktuelle Seite
    uint8_t _currentPage = PAGE_MAIN;
};
