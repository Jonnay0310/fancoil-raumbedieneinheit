/**
 * @file main.cpp
 * @brief Hauptprogramm der Fan-Coil Raumbedieneinheit
 *
 * Koordiniert alle Module:
 * - Nextion Display (HMI)
 * - Modbus RTU (Master + Slave)
 * - Fan-Coil Steuerungslogik
 * - Zeitprogramm-Scheduler
 * - NVS-Einstellungsspeicherung
 * - Watchdog Timer
 *
 * Hardware:
 * - ESP32
 * - Nextion NX8048P070-011C (7" Touchscreen)
 * - RS485 Modbus RTU
 * - DS18B20 oder NTC Temperatursensor
 */

#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <time.h>

#include "config.h"
#include "register_map.h"
#include "nextion_display.h"
#include "modbus_handler.h"
#include "fancoil_controller.h"
#include "scheduler.h"

// ============================================================
// GLOBALE OBJEKTE
// ============================================================

NextionDisplay  display;
ModbusHandler   modbus;
FancoilController controller;
Scheduler       scheduler;
Preferences     nvs;

// ============================================================
// ZEITSTEMPEL FÜR INTERVALLE
// ============================================================

uint32_t lastModbusPoll    = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastControlLoop   = 0;
uint32_t lastSchedulerCheck = 0;
uint32_t lastNvsSave       = 0;

// Flag: Einstellungen wurden geändert → NVS speichern
bool settingsDirty = false;

// ============================================================
// VORWÄRTS-DEKLARATIONEN DER CALLBACKS
// ============================================================

void onSetpointChanged(int16_t value);
void onModeChanged(uint8_t mode);
void onFanSpeedChanged(uint8_t fanSpeed);
void onEcoModeChanged(bool ecoOn);
void onScheduleChanged(uint8_t day, uint8_t slot, uint8_t startH, uint8_t startM,
                        uint8_t endH, uint8_t endM, uint8_t mode);
void onSystemSettingChanged(uint8_t key, int16_t value);
void onModbusHoldingChanged(uint16_t reg, uint16_t value);

// ============================================================
// NVS EINSTELLUNGEN LADEN
// ============================================================

void loadSettings() {
    nvs.begin(NVS_NAMESPACE, true);  // Read-only

    int16_t setpoint   = nvs.getShort(NVS_KEY_SETPOINT,   SETPOINT_DEFAULT);
    uint8_t mode       = nvs.getUChar(NVS_KEY_MODE,        MODE_OFF);
    uint8_t fan        = nvs.getUChar(NVS_KEY_FAN,         FAN_AUTO);
    bool    eco        = nvs.getBool(NVS_KEY_ECO,          false);
    uint8_t tempSrc    = nvs.getUChar(NVS_KEY_TEMP_SRC,    TEMP_SOURCE_LOCAL);
    uint8_t slaveAddr  = nvs.getUChar(NVS_KEY_SLAVE_ADDR,  MODBUS_SLAVE_ADDR_DEFAULT);
    int16_t ecoOffset  = nvs.getShort(NVS_KEY_ECO_OFFSET,  ECO_OFFSET_DEFAULT);
    int16_t frostTemp  = nvs.getShort(NVS_KEY_FROST_TEMP,  FROST_TEMP_DEFAULT);

    nvs.end();

    // Controller-Einstellungen anwenden
    controller.setSetpoint(setpoint);
    controller.setMode(mode);
    controller.setFanSpeed(fan);
    controller.setEcoMode(eco);
    controller.setTempSource(tempSrc);
    controller.setEcoOffset(ecoOffset);
    controller.setFrostTemp(frostTemp);

    // Modbus Holding Register synchronisieren
    modbus.setHoldingReg(REG_SETPOINT,    (uint16_t)setpoint);
    modbus.setHoldingReg(REG_FAN_SPEED,   fan);
    modbus.setHoldingReg(REG_MODE,        mode);
    modbus.setHoldingReg(REG_ECO_MODE,    eco ? 1 : 0);
    modbus.setHoldingReg(REG_TEMP_SOURCE, tempSrc);
    modbus.setHoldingReg(REG_SLAVE_ADDR,  slaveAddr);
    modbus.setHoldingReg(REG_ECO_OFFSET,  (uint16_t)ecoOffset);
    modbus.setHoldingReg(REG_FROST_TEMP,  (uint16_t)frostTemp);

    DEBUG_PRINTLN("[Main] Einstellungen aus NVS geladen");
}

// ============================================================
// NVS EINSTELLUNGEN SPEICHERN
// ============================================================

void saveSettings() {
    nvs.begin(NVS_NAMESPACE, false);  // Read-Write

    nvs.putShort(NVS_KEY_SETPOINT,   controller.getSetpoint());
    nvs.putUChar(NVS_KEY_MODE,       controller.getMode());
    nvs.putUChar(NVS_KEY_FAN,        modbus.getHoldingReg(REG_FAN_SPEED));
    nvs.putBool(NVS_KEY_ECO,         controller.isEcoActive());
    nvs.putUChar(NVS_KEY_TEMP_SRC,   modbus.getHoldingReg(REG_TEMP_SOURCE));
    nvs.putUChar(NVS_KEY_SLAVE_ADDR, modbus.getHoldingReg(REG_SLAVE_ADDR));
    nvs.putShort(NVS_KEY_ECO_OFFSET, modbus.getHoldingReg(REG_ECO_OFFSET));
    nvs.putShort(NVS_KEY_FROST_TEMP, modbus.getHoldingReg(REG_FROST_TEMP));

    nvs.end();

    settingsDirty = false;
    DEBUG_PRINTLN("[Main] Einstellungen in NVS gespeichert");
}

// ============================================================
// AKTUELLEN WOCHENTAG UND UHRZEIT ERMITTELN
// ============================================================

void getCurrentTime(uint8_t& hour, uint8_t& minute, uint8_t& dayOfWeek) {
    // Interne RTC des ESP32 verwenden
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        hour      = timeinfo.tm_hour;
        minute    = timeinfo.tm_min;
        // tm_wday: 0=Sonntag, 1=Montag, ... → umrechnen auf 0=Montag
        dayOfWeek = (timeinfo.tm_wday == 0) ? 6 : (timeinfo.tm_wday - 1);
    } else {
        // Fallback: 08:00 Montag
        hour      = 8;
        minute    = 0;
        dayOfWeek = DAY_MONDAY;
    }
}

// ============================================================
// MODBUS INPUT REGISTER AKTUALISIEREN
// ============================================================

void updateModbusInputRegisters() {
    modbus.setInputReg(REG_ACTUAL_TEMP,
                       (uint16_t)(int16_t)controller.getActualTemp());
    modbus.setInputReg(REG_VALVE_POSITION, controller.getValvePosition());
    modbus.setInputReg(REG_ACTIVE_FAN,     controller.getActiveFanSpeed());
    modbus.setInputReg(REG_ERROR_STATUS,   controller.getErrorStatus());
    modbus.setInputReg(REG_ACTIVE_MODE,    controller.getEffectiveMode());
    modbus.setInputReg(REG_FROST_ACTIVE,   controller.isFrostActive() ? 1 : 0);
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    // Debug-Serial initialisieren
    Serial.begin(115200);
    delay(500);
    DEBUG_PRINTLN("[Main] Fan-Coil Raumbedieneinheit startet...");

    // Watchdog Timer konfigurieren
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);
    DEBUG_PRINTLN("[Main] Watchdog Timer aktiviert");

    // Modbus initialisieren (Slave-Adresse aus NVS lesen)
    nvs.begin(NVS_NAMESPACE, true);
    uint8_t slaveAddr = nvs.getUChar(NVS_KEY_SLAVE_ADDR, MODBUS_SLAVE_ADDR_DEFAULT);
    nvs.end();

    modbus.begin(slaveAddr);
    modbus.onHoldingRegChanged(onModbusHoldingChanged);
    DEBUG_PRINTLN("[Main] Modbus initialisiert");

    // Einstellungen aus NVS laden
    loadSettings();

    // Fan-Coil Controller initialisieren
    uint8_t tempSrc = modbus.getHoldingReg(REG_TEMP_SOURCE);
    controller.begin(tempSrc);
    DEBUG_PRINTLN("[Main] Fan-Coil Controller initialisiert");

    // Scheduler initialisieren
    scheduler.begin();
    DEBUG_PRINTLN("[Main] Scheduler initialisiert");

    // Nextion Display initialisieren
    display.begin();
    display.onSetpointChanged(onSetpointChanged);
    display.onModeChanged(onModeChanged);
    display.onFanSpeedChanged(onFanSpeedChanged);
    display.onEcoModeChanged(onEcoModeChanged);
    display.onScheduleChanged(onScheduleChanged);
    display.onSystemSettingChanged(onSystemSettingChanged);
    DEBUG_PRINTLN("[Main] Display initialisiert");

    DEBUG_PRINTLN("[Main] Initialisierung abgeschlossen");
    esp_task_wdt_reset();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    uint32_t now = millis();

    // Watchdog zurücksetzen
    esp_task_wdt_reset();

    // Nextion Display-Ereignisse verarbeiten
    display.update();

    // Modbus-Aufgaben verarbeiten
    modbus.update();

    // Modbus Master: Remote-Temperatur abfragen (wenn Quelle=Modbus)
    if (modbus.getHoldingReg(REG_TEMP_SOURCE) == TEMP_SOURCE_MODBUS &&
        (now - lastModbusPoll) >= MODBUS_POLL_INTERVAL_MS) {
        lastModbusPoll = now;
        // Adresse 1, Register 0x0010 (Ist-Temperatur) beim übergeordneten Regler
        modbus.requestRemoteTemperature(1, REG_ACTUAL_TEMP);

        if (modbus.hasRemoteTemperature()) {
            controller.setActualTemp(modbus.getRemoteTemperature());
        }
        if (modbus.hasError()) {
            controller.setError(ERR_MODBUS_COMM);
            modbus.clearError();
        } else {
            controller.clearError(ERR_MODBUS_COMM);
        }
    }

    // Steuerlogik ausführen
    if ((now - lastControlLoop) >= CONTROL_LOOP_INTERVAL_MS) {
        lastControlLoop = now;
        controller.update();

        // Modbus Input Register mit aktuellen Werten aktualisieren
        updateModbusInputRegisters();
    }

    // Scheduler prüfen
    if ((now - lastSchedulerCheck) >= SCHEDULER_CHECK_INTERVAL_MS) {
        lastSchedulerCheck = now;

        uint8_t hour, minute, dayOfWeek;
        getCurrentTime(hour, minute, dayOfWeek);
        scheduler.update(hour, minute, dayOfWeek);

        // Scheduler-Modus auf Controller anwenden
        int8_t schedMode = scheduler.getActiveMode();
        if (schedMode >= 0) {
            if (schedMode == SCHED_MODE_ECO) {
                controller.setEcoMode(true);
                if (controller.getMode() == MODE_OFF) {
                    // Kurz Heizen/Auto für Eco-Betrieb
                    controller.setMode(MODE_AUTO);
                }
            } else if (schedMode == SCHED_MODE_OFF) {
                controller.setMode(MODE_OFF);
                controller.setEcoMode(false);
            } else {
                // Komfort
                controller.setEcoMode(false);
                if (controller.getMode() == MODE_OFF) {
                    controller.setMode(MODE_AUTO);
                }
            }
        }
    }

    // Display aktualisieren
    if ((now - lastDisplayUpdate) >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastDisplayUpdate = now;

        display.updateMainScreen(
            controller.getActualTemp(),
            controller.getSetpoint(),
            controller.getEffectiveMode(),
            controller.getActiveFanSpeed(),
            controller.getValvePosition(),
            controller.isEcoActive(),
            controller.isFrostActive(),
            controller.getErrorStatus()
        );
    }

    // Einstellungen in NVS speichern (verzögert, um Flash-Schreibzyklen zu schonen)
    if (settingsDirty && (now - lastNvsSave) >= 5000) {
        lastNvsSave = now;
        saveSettings();
        scheduler.saveToNVS();
    }
}

// ============================================================
// CALLBACKS – Display-Ereignisse
// ============================================================

/**
 * @brief Solltemperatur wurde am Display geändert
 */
void onSetpointChanged(int16_t value) {
    int16_t current = controller.getSetpoint();

    if (value == 9999) {
        // Erhöhen um 0.5°C
        current += 5;
    } else if (value == -9999) {
        // Verringern um 0.5°C
        current -= 5;
    } else {
        current = value;
    }

    current = constrain(current, SETPOINT_MIN, SETPOINT_MAX);
    controller.setSetpoint(current);
    modbus.setHoldingReg(REG_SETPOINT, (uint16_t)current);
    settingsDirty = true;

    DEBUG_PRINTF("[Main] Solltemperatur geändert: %d\n", current);
}

/**
 * @brief Betriebsmodus wurde am Display geändert
 */
void onModeChanged(uint8_t mode) {
    controller.setMode(mode);
    modbus.setHoldingReg(REG_MODE, mode);
    settingsDirty = true;
    DEBUG_PRINTF("[Main] Modus geändert: %d\n", mode);
}

/**
 * @brief Lüfterstufe wurde am Display geändert
 */
void onFanSpeedChanged(uint8_t fanSpeed) {
    controller.setFanSpeed(fanSpeed);
    modbus.setHoldingReg(REG_FAN_SPEED, fanSpeed);
    settingsDirty = true;
    DEBUG_PRINTF("[Main] Lüfterstufe geändert: %d\n", fanSpeed);
}

/**
 * @brief Eco-Modus wurde am Display geändert
 */
void onEcoModeChanged(bool ecoOn) {
    bool current = controller.isEcoActive();
    controller.setEcoMode(!current);  // Toggle
    modbus.setHoldingReg(REG_ECO_MODE, !current ? 1 : 0);
    settingsDirty = true;
    DEBUG_PRINTF("[Main] Eco-Modus: %s\n", !current ? "AN" : "AUS");
}

/**
 * @brief Zeitprogramm wurde am Display geändert
 */
void onScheduleChanged(uint8_t day, uint8_t slot,
                        uint8_t startH, uint8_t startM,
                        uint8_t endH, uint8_t endM, uint8_t mode) {
    scheduler.setTimeSlot(day, slot, startH, startM, endH, endM, mode);
    settingsDirty = true;
    DEBUG_PRINTF("[Main] Zeitprogramm geändert: Tag=%d Slot=%d\n", day, slot);
}

/**
 * @brief Systemeinstellung wurde am Display geändert
 * @param key  Register-Adresse
 * @param value Neuer Wert
 */
void onSystemSettingChanged(uint8_t key, int16_t value) {
    switch (key) {
        case REG_SLAVE_ADDR:
            modbus.setHoldingReg(REG_SLAVE_ADDR, (uint16_t)value);
            break;
        case REG_TEMP_SOURCE:
            controller.setTempSource((uint8_t)value);
            modbus.setHoldingReg(REG_TEMP_SOURCE, (uint16_t)value);
            break;
        case REG_ECO_OFFSET:
            controller.setEcoOffset(value);
            modbus.setHoldingReg(REG_ECO_OFFSET, (uint16_t)value);
            break;
        case REG_FROST_TEMP:
            controller.setFrostTemp(value);
            modbus.setHoldingReg(REG_FROST_TEMP, (uint16_t)value);
            break;
        default:
            break;
    }
    settingsDirty = true;
}

// ============================================================
// CALLBACKS – Modbus-Ereignisse
// ============================================================

/**
 * @brief Ein Holding Register wurde vom übergeordneten Regler beschrieben
 */
void onModbusHoldingChanged(uint16_t reg, uint16_t value) {
    switch (reg) {
        case REG_SETPOINT:
            controller.setSetpoint((int16_t)value);
            break;
        case REG_FAN_SPEED:
            controller.setFanSpeed((uint8_t)value);
            break;
        case REG_MODE:
            controller.setMode((uint8_t)value);
            break;
        case REG_ECO_MODE:
            controller.setEcoMode(value != 0);
            break;
        case REG_TEMP_SOURCE:
            controller.setTempSource((uint8_t)value);
            break;
        case REG_ECO_OFFSET:
            controller.setEcoOffset((int16_t)value);
            break;
        case REG_FROST_TEMP:
            controller.setFrostTemp((int16_t)value);
            break;
        default:
            break;
    }
    settingsDirty = true;

    DEBUG_PRINTF("[Main] Modbus Holding Reg %d = %d\n", reg, value);
}
