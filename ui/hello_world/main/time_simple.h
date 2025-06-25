#ifndef TIME_SIMPLE_H
#define TIME_SIMPLE_H

#include <stdint.h>  // for uint8_t

/**
 * Custom data type used for setting the wakeup window.
 * It is a struct consisting of hour and minute.
 * Used in hello_world_main.c and screen.c
 * 
 * Looking back we should have used timestamps in hello_world_main.c and 
 * screen.c and convert them to a human readable format (HH:MM).
 * This would prevent bugs regarding wrong day of month.
 */
typedef struct {
    uint8_t hour;
    uint8_t minute;
} TimeSimple;

#endif // TIME_SIMPLE_H
