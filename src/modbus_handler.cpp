/**
 * @file modbus_handler.cpp
 * @brief Implementierung des Modbus RTU Master & Slave Handlers
 */

#include "modbus_handler.h"

// Singleton-Instanz für statische Callbacks
ModbusHandler* ModbusHandler::_instance = nullptr;

// ============================================================
// INITIALISIERUNG
// ============================================================

void ModbusHandler::begin(uint8_t slaveAddr) {
    _instance = this;

    // Standard-Werte für Holding Register setzen
    _holdingRegs[REG_SETPOINT]    = SETPOINT_DEFAULT;
    _holdingRegs[REG_FAN_SPEED]   = FAN_AUTO;
    _holdingRegs[REG_MODE]        = MODE_OFF;
    _holdingRegs[REG_ECO_MODE]    = 0;
    _holdingRegs[REG_TEMP_SOURCE] = TEMP_SOURCE_LOCAL;
    _holdingRegs[REG_SLAVE_ADDR]  = slaveAddr;
    _holdingRegs[REG_ECO_OFFSET]  = ECO_OFFSET_DEFAULT;
    _holdingRegs[REG_FROST_TEMP]  = FROST_TEMP_DEFAULT;

    // Input Register initialisieren
    for (uint8_t i = 0; i < INPUT_REG_COUNT; i++) {
        _inputRegs[i] = 0;
    }

    // RS485 UART initialisieren
    MODBUS_SERIAL.begin(MODBUS_BAUDRATE, SERIAL_8N1,
                        MODBUS_RX_PIN, MODBUS_TX_PIN);

    // Modbus RTU initialisieren (Master + Slave)
    _modbus.begin(&MODBUS_SERIAL, MODBUS_DE_RE_PIN);
    _modbus.master();

    // Als Slave konfigurieren
    _modbus.slave(slaveAddr);

    // Holding Register im Modbus-Stack registrieren
    for (uint8_t i = 0; i < HOLDING_REG_COUNT; i++) {
        _modbus.addHreg(i, _holdingRegs[i]);
    }

    // Input Register registrieren (Startadresse INPUT_REG_START = 0x0010 = 16)
    for (uint8_t i = 0; i < INPUT_REG_COUNT; i++) {
        _modbus.addIreg(INPUT_REG_START + i, _inputRegs[i]);
    }

    // Callback für Holding-Register-Schreibzugriffe
    _modbus.onSetHreg(0, cbHoldingWrite, HOLDING_REG_COUNT);

    DEBUG_PRINTF("[Modbus] Initialisiert als Slave Adresse %d\n", slaveAddr);
}

// ============================================================
// UPDATE – regelmäßig aufrufen
// ============================================================

void ModbusHandler::update() {
    _modbus.task();
}

// ============================================================
// HOLDING REGISTER LESEN/SCHREIBEN
// ============================================================

uint16_t ModbusHandler::getHoldingReg(uint16_t reg) {
    if (reg < HOLDING_REG_COUNT) {
        return _holdingRegs[reg];
    }
    return 0;
}

void ModbusHandler::setHoldingReg(uint16_t reg, uint16_t value) {
    if (reg < HOLDING_REG_COUNT) {
        _holdingRegs[reg] = value;
        // Auch im Modbus-Stack aktualisieren
        _modbus.Hreg(reg, value);
    }
}

// ============================================================
// INPUT REGISTER SCHREIBEN/LESEN
// ============================================================

void ModbusHandler::setInputReg(uint16_t reg, uint16_t value) {
    if (reg >= INPUT_REG_START && reg < INPUT_REG_START + INPUT_REG_COUNT) {
        uint8_t idx = reg - INPUT_REG_START;
        _inputRegs[idx] = value;
        // Im Modbus-Stack aktualisieren
        _modbus.Ireg(reg, value);
    }
}

uint16_t ModbusHandler::getInputReg(uint16_t reg) {
    if (reg >= INPUT_REG_START && reg < INPUT_REG_START + INPUT_REG_COUNT) {
        return _inputRegs[reg - INPUT_REG_START];
    }
    return 0;
}

// ============================================================
// MASTER: REMOTE TEMPERATUR ABFRAGEN
// ============================================================

bool ModbusHandler::requestRemoteTemperature(uint8_t remoteAddr, uint16_t remoteReg) {
    uint16_t result[1];
    bool success = _modbus.readIreg(remoteAddr, remoteReg, result, 1, cbMasterResult);
    if (!success) {
        DEBUG_PRINTLN("[Modbus] Master-Anfrage fehlgeschlagen");
        _hasError = true;
    }
    return success;
}

bool ModbusHandler::hasRemoteTemperature() const {
    return _remoteTempValid;
}

int16_t ModbusHandler::getRemoteTemperature() const {
    return _remoteTemp;
}

bool ModbusHandler::hasError() const {
    return _hasError;
}

void ModbusHandler::clearError() {
    _hasError = false;
}

// ============================================================
// CALLBACK REGISTRIERUNG
// ============================================================

void ModbusHandler::onHoldingRegChanged(void (*cb)(uint16_t, uint16_t)) {
    _cbHoldingChanged = cb;
}

// ============================================================
// STATISCHE CALLBACKS
// ============================================================

/**
 * @brief Wird aufgerufen, wenn der übergeordnete Regler ein Holding-Register schreibt
 */
uint16_t ModbusHandler::cbHoldingWrite(TRegister* reg, uint16_t val) {
    if (_instance == nullptr) return val;

    uint16_t regAddr = reg->address.address;

    // Grenzwertprüfung
    switch (regAddr) {
        case REG_SETPOINT:
            val = constrain(val, SETPOINT_MIN, SETPOINT_MAX);
            break;
        case REG_FAN_SPEED:
            val = constrain(val, 0, 3);
            break;
        case REG_MODE:
            val = constrain(val, 0, 3);
            break;
        case REG_ECO_MODE:
            val = constrain(val, 0, 1);
            break;
        case REG_TEMP_SOURCE:
            val = constrain(val, 0, 1);
            break;
        case REG_SLAVE_ADDR:
            val = constrain(val, 1, 247);
            break;
        case REG_ECO_OFFSET:
            val = constrain(val, 10, 50);
            break;
        case REG_FROST_TEMP:
            val = constrain(val, 30, 100);
            break;
        default:
            break;
    }

    // Lokale Kopie aktualisieren
    if (regAddr < HOLDING_REG_COUNT) {
        _instance->_holdingRegs[regAddr] = val;
    }

    // Callback aufrufen
    if (_instance->_cbHoldingChanged) {
        _instance->_cbHoldingChanged(regAddr, val);
    }

    DEBUG_PRINTF("[Modbus] Holding Reg %d = %d\n", regAddr, val);
    return val;
}

/**
 * @brief Callback für Master-Anfrage (Remote-Temperatur)
 */
bool ModbusHandler::cbMasterResult(Modbus::ResultCode result,
                                    uint16_t transactionId, void* data) {
    if (_instance == nullptr) return true;

    if (result == Modbus::EX_SUCCESS) {
        // Daten in data-Zeiger auslesen (uint16_t Array)
        uint16_t* response = (uint16_t*)data;
        _instance->_remoteTemp = (int16_t)response[0];
        _instance->_remoteTempValid = true;
        _instance->_hasError = false;
        DEBUG_PRINTF("[Modbus] Remote Temperatur: %d (×10)\n",
                     _instance->_remoteTemp);
    } else {
        _instance->_remoteTempValid = false;
        _instance->_hasError = true;
        DEBUG_PRINTF("[Modbus] Master Fehler: 0x%02X\n", result);
    }
    return true;
}
