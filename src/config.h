#pragma once

/**
 * @file config.h
 * @brief Konfigurationsdatei für Fan-Coil Raumbedieneinheit
 *
 * Enthält alle Pin-Definitionen, Modbus-Einstellungen und Standard-Werte.
 * Alle Werte können hier zentral angepasst werden.
 */

// ============================================================
// UART / SERIELL EINSTELLUNGEN
// ============================================================

// Nextion Display – Serial2
#define NEXTION_SERIAL       Serial2
#define NEXTION_RX_PIN       16
#define NEXTION_TX_PIN       17
#define NEXTION_BAUDRATE     9600

// RS485 / Modbus – Serial1
#define MODBUS_SERIAL        Serial1
#define MODBUS_TX_PIN        25
#define MODBUS_RX_PIN        26
#define MODBUS_DE_RE_PIN     27      // Direction Enable / Receive Enable Pin
#define MODBUS_BAUDRATE      9600

// ============================================================
// TEMPERATUR-SENSOR PINS
// ============================================================

// DS18B20 OneWire Sensor
#define ONEWIRE_PIN          4

// NTC Analogeingang (falls DS18B20 nicht verwendet)
#define NTC_PIN              34
#define NTC_SERIES_RESISTOR  10000   // 10kΩ Vorwiderstand
#define NTC_NOMINAL_RES      10000   // 10kΩ bei 25°C
#define NTC_NOMINAL_TEMP     25      // Nenntemperatur in °C
#define NTC_BCOEFFICIENT     3950    // B-Koeffizient des NTC

// ============================================================
// MODBUS-EINSTELLUNGEN
// ============================================================

// Standard Slave-Adresse (1–247)
#define MODBUS_SLAVE_ADDR_DEFAULT  1

// Modbus Timeout in Millisekunden
#define MODBUS_TIMEOUT_MS    1000

// ============================================================
// TEMPERATUR-EINSTELLUNGEN
// ============================================================

// Solltemperatur Bereich (×10 für Festkomma, z.B. 220 = 22.0°C)
#define SETPOINT_MIN         180     // 18.0°C
#define SETPOINT_MAX         280     // 28.0°C
#define SETPOINT_DEFAULT     220     // 22.0°C

// Eco-Absenkung Standard (×10, z.B. 20 = 2.0°C)
#define ECO_OFFSET_DEFAULT   20

// Frostschutz-Temperatur Standard (×10, z.B. 50 = 5.0°C)
#define FROST_TEMP_DEFAULT   50

// ============================================================
// LÜFTERSTUFEN
// ============================================================

// 0=Auto, 1=Stufe 1, 2=Stufe 2, 3=Stufe 3
#define FAN_AUTO             0
#define FAN_SPEED_1          1
#define FAN_SPEED_2          2
#define FAN_SPEED_3          3

// ============================================================
// BETRIEBSMODI
// ============================================================

#define MODE_OFF             0
#define MODE_HEAT            1
#define MODE_COOL            2
#define MODE_AUTO            3

// ============================================================
// TEMPERATURQUELLE
// ============================================================

#define TEMP_SOURCE_LOCAL    0       // Lokaler Sensor (DS18B20 oder NTC)
#define TEMP_SOURCE_MODBUS   1       // Über Modbus vom übergeordneten Regler

// ============================================================
// ZEITINTERVALLE (Millisekunden)
// ============================================================

// Sensor-Abfrageintervall
#define SENSOR_READ_INTERVAL_MS     2000

// Modbus Master Abfrageintervall
#define MODBUS_POLL_INTERVAL_MS     1000

// Nextion Aktualisierungsintervall
#define DISPLAY_UPDATE_INTERVAL_MS  500

// Steuerlogik Ausführungsintervall
#define CONTROL_LOOP_INTERVAL_MS    1000

// Scheduler Prüfintervall (jede Minute)
#define SCHEDULER_CHECK_INTERVAL_MS 60000

// ============================================================
// NVS / EEPROM NAMESPACES & SCHLÜSSEL
// ============================================================

#define NVS_NAMESPACE        "fancoil"
#define NVS_KEY_SETPOINT     "setpoint"
#define NVS_KEY_MODE         "mode"
#define NVS_KEY_FAN          "fan"
#define NVS_KEY_ECO          "eco"
#define NVS_KEY_TEMP_SRC     "tempsrc"
#define NVS_KEY_SLAVE_ADDR   "slaveaddr"
#define NVS_KEY_ECO_OFFSET   "ecooffset"
#define NVS_KEY_FROST_TEMP   "frosttemp"

// ============================================================
// WATCHDOG TIMER
// ============================================================

// Watchdog Timeout in Sekunden
#define WDT_TIMEOUT_S        10

// ============================================================
// DEBUG
// ============================================================

// Debug-Ausgabe aktivieren (1=an, 0=aus)
#define DEBUG_ENABLED        1

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif
