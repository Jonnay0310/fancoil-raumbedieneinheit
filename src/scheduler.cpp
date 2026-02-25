/**
 * @file scheduler.cpp
 * @brief Implementierung des Zeitprogramm-Schedulers
 */

#include "scheduler.h"
#include <Preferences.h>

// ============================================================
// INITIALISIERUNG
// ============================================================

void Scheduler::begin() {
    // Alle Zeitfenster initialisieren (deaktiviert)
    for (uint8_t day = 0; day < DAYS_PER_WEEK; day++) {
        for (uint8_t slot = 0; slot < SLOTS_PER_DAY; slot++) {
            _schedule[day][slot] = {0, 0, 23, 59, SCHED_MODE_COMFORT, false};
        }
    }

    // Aus NVS laden
    if (!loadFromNVS()) {
        // Standard-Wochenprogramm: Werktage 6:00–22:00 Komfort
        for (uint8_t day = DAY_MONDAY; day <= DAY_FRIDAY; day++) {
            setTimeSlot(day, 0, 6, 0, 22, 0, SCHED_MODE_COMFORT, true);
        }
        // Wochenende 8:00–23:00 Komfort
        setTimeSlot(DAY_SATURDAY, 0, 8, 0, 23, 0, SCHED_MODE_COMFORT, true);
        setTimeSlot(DAY_SUNDAY,   0, 8, 0, 23, 0, SCHED_MODE_COMFORT, true);
        DEBUG_PRINTLN("[Scheduler] Standard-Wochenprogramm geladen");
    }

    DEBUG_PRINTLN("[Scheduler] Scheduler initialisiert");
}

// ============================================================
// UPDATE – regelmäßig aufrufen
// ============================================================

void Scheduler::update(uint8_t currentHour, uint8_t currentMin, uint8_t currentDay) {
    if (!_schedulerEnabled) {
        _activeMode = -1;
        return;
    }

    if (currentDay >= DAYS_PER_WEEK) return;

    // Aktuell aktives Zeitfenster suchen
    _activeMode = -1;  // kein Zeitfenster aktiv
    for (uint8_t slot = 0; slot < SLOTS_PER_DAY; slot++) {
        const TimeSlot& ts = _schedule[currentDay][slot];
        if (ts.enabled && isTimeInSlot(currentHour, currentMin, ts)) {
            _activeMode = ts.mode;
            break;
        }
    }

    DEBUG_PRINTF("[Scheduler] Tag=%d Zeit=%02d:%02d Modus=%d\n",
                 currentDay, currentHour, currentMin, _activeMode);
}

// ============================================================
// GETTER
// ============================================================

int8_t Scheduler::getActiveMode() const {
    return _activeMode;
}

bool Scheduler::isSchedulerActive() const {
    return _schedulerEnabled && (_activeMode >= 0);
}

// ============================================================
// SETTER
// ============================================================

void Scheduler::setEnabled(bool enabled) {
    _schedulerEnabled = enabled;
    DEBUG_PRINTF("[Scheduler] %s\n", enabled ? "Aktiviert" : "Deaktiviert");
}

void Scheduler::setTimeSlot(uint8_t day, uint8_t slot,
                              uint8_t startH, uint8_t startM,
                              uint8_t endH, uint8_t endM,
                              uint8_t mode, bool enabled) {
    if (day >= DAYS_PER_WEEK || slot >= SLOTS_PER_DAY) return;

    _schedule[day][slot] = {startH, startM, endH, endM, mode, enabled};
    DEBUG_PRINTF("[Scheduler] Setze Slot: Tag=%d Slot=%d %02d:%02d-%02d:%02d Modus=%d\n",
                 day, slot, startH, startM, endH, endM, mode);
}

const TimeSlot* Scheduler::getTimeSlot(uint8_t day, uint8_t slot) const {
    if (day >= DAYS_PER_WEEK || slot >= SLOTS_PER_DAY) return nullptr;
    return &_schedule[day][slot];
}

// ============================================================
// NVS SPEICHERN / LADEN
// ============================================================

void Scheduler::saveToNVS() {
    Preferences prefs;
    prefs.begin("scheduler", false);

    // Zeitfenster-Daten als Blob speichern
    prefs.putBytes("schedule", _schedule, sizeof(_schedule));
    prefs.putBool("enabled", _schedulerEnabled);
    prefs.end();

    DEBUG_PRINTLN("[Scheduler] Wochenprogramm in NVS gespeichert");
}

bool Scheduler::loadFromNVS() {
    Preferences prefs;
    prefs.begin("scheduler", true);  // Nur lesen

    size_t schSize = prefs.getBytesLength("schedule");
    if (schSize != sizeof(_schedule)) {
        prefs.end();
        return false;
    }

    prefs.getBytes("schedule", _schedule, sizeof(_schedule));
    _schedulerEnabled = prefs.getBool("enabled", false);
    prefs.end();

    DEBUG_PRINTLN("[Scheduler] Wochenprogramm aus NVS geladen");
    return true;
}

// ============================================================
// HILFSFUNKTIONEN
// ============================================================

bool Scheduler::isTimeInSlot(uint8_t hour, uint8_t min,
                               const TimeSlot& slot) const {
    uint16_t current = toMinutes(hour, min);
    uint16_t start   = toMinutes(slot.startHour, slot.startMin);
    uint16_t end     = toMinutes(slot.endHour, slot.endMin);

    if (start <= end) {
        // Normales Zeitfenster (z.B. 06:00–22:00)
        return (current >= start && current < end);
    } else {
        // Zeitfenster über Mitternacht (z.B. 22:00–06:00)
        return (current >= start || current < end);
    }
}

uint16_t Scheduler::toMinutes(uint8_t hour, uint8_t min) const {
    return (uint16_t)(hour * 60 + min);
}

// ============================================================
// STATISCHE STRINGS
// ============================================================

const char* Scheduler::getDayName(uint8_t day) {
    static const char* days[] = {
        "Montag", "Dienstag", "Mittwoch", "Donnerstag",
        "Freitag", "Samstag", "Sonntag"
    };
    if (day < DAYS_PER_WEEK) return days[day];
    return "???";
}

const char* Scheduler::getScheduleModeString(uint8_t mode) {
    switch (mode) {
        case SCHED_MODE_COMFORT: return "Komfort";
        case SCHED_MODE_ECO:     return "Eco";
        case SCHED_MODE_OFF:     return "Aus";
        default:                 return "???";
    }
}
