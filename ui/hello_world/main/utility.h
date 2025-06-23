#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include "time_simple.h"

typedef enum {
    ACTION_NONE,
    ACTION_TRIGGER_ALARM,
    ACTION_UNKNOWN
} server_action_t;

server_action_t parse_server_response(const char *json, int64_t *ts_out);

const char* ts_to_hhmmss(int64_t ts_ms);

int64_t time_simple_to_timestamp(TimeSimple time_simple);

#endif // UTILITY_H