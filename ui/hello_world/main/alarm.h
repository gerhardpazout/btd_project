#pragma once

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


