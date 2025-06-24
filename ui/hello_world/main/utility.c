#include "utility.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

server_action_t parse_server_response(const char *json, double *low_out, double *high_out) {
    if (!json || !low_out || !high_out) return ACTION_UNKNOWN;

    *low_out = 0;
    *high_out = 0;

    if (strstr(json, "\"action\": \"TRIGGER_ALARM\"")) {
        const char *low_key = "\"threshold_low\": ";
        const char *low_pos = strstr(json, low_key);
        if (low_pos) {
            *low_out = strtod(low_pos + strlen(low_key), NULL);
        }

        const char *high_key = "\"threshold_high\": ";
        const char *high_pos = strstr(json, high_key);
        if (high_pos) {
            *high_out = strtod(high_pos + strlen(high_key), NULL);
        }

        return ACTION_TRIGGER_ALARM;
    }

    if (strstr(json, "\"action\": \"NONE\"")) {
        return ACTION_NONE;
    }

    return ACTION_UNKNOWN;
}

int64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void ts_to_hhmmss_str(int64_t ts_ms, char *out, size_t len) {
    time_t seconds = ts_ms / 1000;
    struct tm timeinfo;
    localtime_r(&seconds, &timeinfo);
    strftime(out, len, "%H:%M:%S", &timeinfo);
}

int64_t time_simple_to_timestamp(TimeSimple time_simple) {
    time_t now = time(NULL);
    struct tm timeinfo;

    localtime_r(&now, &timeinfo);  // get current date
    timeinfo.tm_hour = time_simple.hour;
    timeinfo.tm_min  = time_simple.minute;
    timeinfo.tm_sec  = 0;

    time_t result_sec = mktime(&timeinfo);  // convert to UNIX timestamp (seconds)
    return (int64_t)result_sec * 1000;      // convert to ms
}