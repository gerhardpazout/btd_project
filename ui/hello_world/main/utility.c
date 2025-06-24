#include "utility.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

server_action_t parse_server_response(const char *json, int64_t *ts_out) {
    if (!json || !ts_out) return ACTION_UNKNOWN;

    *ts_out = 0;

    if (strstr(json, "\"action\": \"TRIGGER_ALARM\"")) {
        const char *ts_key = "\"timestamp\": ";
        const char *ts_pos = strstr(json, ts_key);
        if (ts_pos) {
            *ts_out = strtoll(ts_pos + strlen(ts_key), NULL, 10);
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