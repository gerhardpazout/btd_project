#include "time_simple.h"
#include <stdbool.h>

/* 
 * This file contains shared globals that can be used in any files.
 * It is a quick and dirty solution, but we thought it's ok for the scope of the lecture.
 */

extern TimeSimple alarm_start;
extern TimeSimple alarm_end;
extern uint8_t alarm_index;

extern bool is_alarm_set;