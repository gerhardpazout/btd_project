#pragma once

#include <stdint.h>

/**
 * @brief Initializes the beeper (GPIO and PWM)
 * Must be called before using beep().
 */
void alarm_init(void);

/**
 * @brief Triggers a short beep sound.
 * 
 * @param duration_ms Duration of the beep in milliseconds.
 */
void beep(int duration_ms);

static void alarm_task(void *arg);

void startAlarmAt(int64_t ts);

static float compute_variance(float *data, int len);

void startAlarmWhenMovementInRange(double threshold_low, double threshold_high);