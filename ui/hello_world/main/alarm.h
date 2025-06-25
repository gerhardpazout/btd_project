#pragma once

#include <stdint.h>

/**
 * Initializes the beeper (GPIO and PWM)
 * Must be called before using beep().
 */
void alarm_init(void);

/**
 * Triggers a short beep sound.
 * 
 * @param duration_ms Duration of the beep in milliseconds.
 */
void beep(int duration_ms);

/**
 * Task that starts an alarm at a given timestamp (DEPRECATED!!!)
 * 
 * @param duration_ms Duration of the beep in milliseconds.
 */
static void alarm_task(void *arg);

/**
 * Starts alarm at given UNIX timestamp (ms) (DEPRECATED!!!) 
 */
void startAlarmAt(int64_t ts);

/**
 * Calculates the variance of movement data
 */
static float compute_variance(float *data, int len);

/**
 * Starts an alarm (beep sound) if the ESPs movement exceeds a certain threshold
 */
void startAlarmWhenMovementInRange(double threshold_low, double threshold_high);