#pragma once

/**
 * @file modbus_handler.h
 * @brief Modbus RTU Master & Slave Handler für Fan-Coil Raumbedieneinheit
 *
 * Der ESP32 agiert gleichzeitig als:
 * - Modbus SLAVE: empfängt Befehle vom übergeordneten Regler (SPS/DDC)
 * - Modbus MASTER: liest ggf. Ist-Temperatur vom übergeordneten Regler
 *
 * Verwendet die Bibliothek: emelianov/modbus-esp8266
 */

#include <Arduino.h>
#include <ModbusRTU.h>
#include "config.h"
#include "register_map.h"

// ============================================================
// KLASSE ModbusHandler
// ============================================================

class ModbusHandler {
public:
    /**
     * @brief Initialisiert Modbus RTU Master & Slave
     * @param slaveAddr Modbus Slave-Adresse (1–247)
     */
    void begin(uint8_t slaveAddr = MODBUS_SLAVE_ADDR_DEFAULT);

    /**
     * @brief Muss regelmäßig im Loop aufgerufen werden (verarbeitet Modbus-Frames)
     */
    void update();

    // ---- Holding Register (R/W) ----

    /**
     * @brief Liest einen Holding-Register-Wert
     * @param reg Registeradresse (REG_xxx aus register_map.h)
     * @return Registerwert
     */
    uint16_t getHoldingReg(uint16_t reg);

    /**
     * @brief Setzt einen Holding-Register-Wert (lokal)
     * @param reg   Registeradresse
     * @param value Neuer Wert
     */
    void setHoldingReg(uint16_t reg, uint16_t value);

    // ---- Input Register (Read Only) ----

    /**
     * @brief Setzt einen Input-Register-Wert (wird vom Slave bereitgestellt)
     * @param reg   Registeradresse (REG_xxx aus register_map.h)
     * @param value Neuer Wert
     */
    void setInputReg(uint16_t reg, uint16_t value);

    /**
     * @brief Liest einen Input-Register-Wert
     * @param reg Registeradresse
     * @return Registerwert
     */
    uint16_t getInputReg(uint16_t reg);

    // ---- Master-Funktion: Ist-Temperatur abfragen ----

    /**
     * @brief Fragt die Ist-Temperatur vom übergeordneten Regler ab (als Master)
     * @param remoteAddr Modbus-Adresse des übergeordneten Reglers
     * @param remoteReg  Registeradresse beim übergeordneten Regler
     * @return true wenn Anfrage gesendet wurde
     */
    bool requestRemoteTemperature(uint8_t remoteAddr, uint16_t remoteReg);

    /**
     * @brief Gibt zurück, ob eine gültige Remote-Temperatur vorliegt
     */
    bool hasRemoteTemperature() const;

    /**
     * @brief Gibt die zuletzt empfangene Remote-Temperatur zurück (×10)
     */
    int16_t getRemoteTemperature() const;

    /**
     * @brief Gibt zurück, ob ein Modbus-Fehler vorliegt
     */
    bool hasError() const;

    /**
     * @brief Setzt den Fehlerstatus zurück
     */
    void clearError();

    // ---- Callback für Holding-Register-Änderungen ----

    /**
     * @brief Registriert Callback für Änderungen an Holding-Registern
     * Der Callback wird aufgerufen, wenn der übergeordnete Regler einen Wert schreibt.
     * @param cb Callback-Funktion: void(uint16_t reg, uint16_t value)
     */
    void onHoldingRegChanged(void (*cb)(uint16_t, uint16_t));

private:
    ModbusRTU _modbus;

    // Lokale Kopien der Register-Werte
    uint16_t _holdingRegs[HOLDING_REG_COUNT];
    uint16_t _inputRegs[INPUT_REG_COUNT];

    // Remote Temperatur (vom Master-Request)
    int16_t  _remoteTemp    = 0;
    bool     _remoteTempValid = false;
    bool     _hasError      = false;

    // Letzter Zeitpunkt des Master-Requests
    uint32_t _lastMasterRequest = 0;

    // Callback
    void (*_cbHoldingChanged)(uint16_t, uint16_t) = nullptr;

    // Statischer Callback für Modbus-Bibliothek (Holding Register Write)
    static uint16_t cbHoldingWrite(TRegister* reg, uint16_t val);
    static ModbusHandler* _instance;  // Singleton-Referenz für statischen Callback

    // Master-Callback: Antwort auf Remote-Temperatur-Anfrage
    static bool cbMasterResult(Modbus::ResultCode result, uint16_t transactionId, void* data);
};
