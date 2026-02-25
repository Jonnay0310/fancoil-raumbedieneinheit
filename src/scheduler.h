#pragma once

/**
 * @file scheduler.h
 * @brief Zeitprogramm / Wochenprogramm für Fan-Coil Raumbedieneinheit
 *
 * Verwaltet ein Wochenprogramm mit bis zu 4 Zeitfenstern pro Tag.
 * Jedes Zeitfenster hat:
 * - Startzeit (Stunde:Minute)
 * - Endzeit (Stunde:Minute)
 * - Modus (Komfort / Eco / Aus)
 *
 * Die aktuelle Uhrzeit wird aus dem internen RTC des ESP32 verwendet
 * (wird per NTP synchronisiert oder manuell gesetzt).
 */

#include <Arduino.h>
#include "config.h"

// ============================================================
// KONSTANTEN
// ============================================================

/** Anzahl der Wochentage */
#define DAYS_PER_WEEK        7

/** Maximale Anzahl an Zeitfenstern pro Tag */
#define SLOTS_PER_DAY        4

// Zeitfenster-Modi
#define SCHED_MODE_COMFORT   0   // Komfort: Normale Solltemperatur
#define SCHED_MODE_ECO       1   // Eco: Abgesenkte Solltemperatur
#define SCHED_MODE_OFF       2   // Aus: Gerät aus (Frostschutz aktiv)

// Wochentage (Montag = 0, Sonntag = 6)
#define DAY_MONDAY           0
#define DAY_TUESDAY          1
#define DAY_WEDNESDAY        2
#define DAY_THURSDAY         3
#define DAY_FRIDAY           4
#define DAY_SATURDAY         5
#define DAY_SUNDAY           6

// ============================================================
// DATENSTRUKTUR: Zeitfenster
// ============================================================

/**
 * @brief Beschreibt ein einzelnes Zeitfenster
 */
struct TimeSlot {
    uint8_t startHour;   ///< Startstunde (0–23)
    uint8_t startMin;    ///< Startminute (0–59)
    uint8_t endHour;     ///< Endstunde (0–23)
    uint8_t endMin;      ///< Endminute (0–59)
    uint8_t mode;        ///< Modus (SCHED_MODE_xxx)
    bool    enabled;     ///< Zeitfenster aktiv
};

// ============================================================
// KLASSE Scheduler
// ============================================================

class Scheduler {
public:
    /**
     * @brief Initialisiert den Scheduler
     * Lädt das Wochenprogramm aus dem NVS (falls vorhanden)
     */
    void begin();

    /**
     * @brief Prüft das Zeitprogramm und gibt den aktuellen Modus zurück
     * Muss regelmäßig aufgerufen werden.
     * @param currentHour Aktuelle Stunde (0–23)
     * @param currentMin  Aktuelle Minute (0–59)
     * @param currentDay  Aktueller Wochentag (0=Montag, 6=Sonntag)
     */
    void update(uint8_t currentHour, uint8_t currentMin, uint8_t currentDay);

    /**
     * @brief Gibt den aktuell aktiven Scheduler-Modus zurück
     * @return SCHED_MODE_COMFORT, SCHED_MODE_ECO oder SCHED_MODE_OFF
     *         Gibt -1 zurück, wenn kein Zeitfenster aktiv ist (manueller Modus)
     */
    int8_t getActiveMode() const;

    /**
     * @brief Gibt zurück, ob der Scheduler aktiv ist (Zeitprogramm läuft)
     */
    bool isSchedulerActive() const;

    /**
     * @brief Aktiviert/deaktiviert den Scheduler
     * @param enabled true = Zeitprogramm aktiv, false = manueller Betrieb
     */
    void setEnabled(bool enabled);

    /**
     * @brief Setzt ein Zeitfenster für einen bestimmten Tag
     * @param day       Wochentag (0–6)
     * @param slot      Zeitfenster-Nummer (0–3)
     * @param startH    Startstunde
     * @param startM    Startminute
     * @param endH      Endstunde
     * @param endM      Endminute
     * @param mode      Modus (SCHED_MODE_xxx)
     * @param enabled   Zeitfenster aktivieren
     */
    void setTimeSlot(uint8_t day, uint8_t slot,
                     uint8_t startH, uint8_t startM,
                     uint8_t endH, uint8_t endM,
                     uint8_t mode, bool enabled = true);

    /**
     * @brief Liest ein Zeitfenster aus
     * @param day  Wochentag (0–6)
     * @param slot Zeitfenster-Nummer (0–3)
     * @return Zeiger auf das TimeSlot-Objekt (const)
     */
    const TimeSlot* getTimeSlot(uint8_t day, uint8_t slot) const;

    /**
     * @brief Speichert das Wochenprogramm ins NVS
     */
    void saveToNVS();

    /**
     * @brief Lädt das Wochenprogramm aus dem NVS
     * @return true wenn erfolgreich geladen
     */
    bool loadFromNVS();

    /**
     * @brief Gibt den Tagnamen zurück (für Anzeige)
     * @param day Wochentag (0–6)
     */
    static const char* getDayName(uint8_t day);

    /**
     * @brief Gibt den Modusnamen zurück (für Anzeige)
     * @param mode SCHED_MODE_xxx
     */
    static const char* getScheduleModeString(uint8_t mode);

private:
    // Wochenprogramm (7 Tage × 4 Zeitfenster)
    TimeSlot _schedule[DAYS_PER_WEEK][SLOTS_PER_DAY];

    // Aktueller Scheduler-Modus
    int8_t  _activeMode       = -1;  // -1 = kein Zeitfenster aktiv
    bool    _schedulerEnabled = false;

    /**
     * @brief Prüft ob eine Uhrzeit innerhalb eines Zeitfensters liegt
     * @param hour      Stunde (0–23)
     * @param min       Minute (0–59)
     * @param slot      Zu prüfendes Zeitfenster
     * @return true wenn innerhalb des Zeitfensters
     */
    bool isTimeInSlot(uint8_t hour, uint8_t min, const TimeSlot& slot) const;

    /**
     * @brief Vergleicht zwei Uhrzeiten (gibt Minuten seit Mitternacht zurück)
     */
    uint16_t toMinutes(uint8_t hour, uint8_t min) const;
};
