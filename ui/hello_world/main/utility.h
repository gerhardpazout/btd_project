#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include "time_simple.h"
#include <stddef.h>

/**
 * Types of actions the server can send to the ESP to call functions on it.
 * ACTION_TRIGGER_ALARM is used to trigger the alarm
 */
typedef enum {
    ACTION_NONE,
    ACTION_TRIGGER_ALARM,
    ACTION_UNKNOWN
} server_action_t;

/**
 * Takes server response (JSON-String) and tries to set thresholds for movement (low, high)
 */
server_action_t parse_server_response(const char *json, double *low_out, double *high_out);

/**
 * Returns current system time in UNIX timestamp (ms)
 */
int64_t now_ms(void);

/**
 * Converts UNIX timestamp (ms) to a string. Works with pointers!
 */
void ts_to_hhmmss_str(int64_t ts_ms, char *out, size_t len);

/**
 * Converts custom type TimeSimple (struct with "hours" and "minutes") to a UNIX timestamp (ms)
 */
int64_t time_simple_to_timestamp(TimeSimple time_simple);

#endif // UTILITY_H