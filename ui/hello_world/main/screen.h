#pragma once
#include "time_simple.h"

/**
 * Lists all spiffs files. Used for debugging
 */
void list_spiffs_files();

/**
 * Mounts spiffs
 */
void init_spiffs(void);

/**
 * Initializes ESP's screen so its ready for use. 
 * Call before doing anything screen related.
 */
void init_screen();

/**
 * Renders basic "Hello World" on screen to get started.
 * Supposed to be more of a debugging tool.
 */
void render_screen();

/**
 * Updates the screen using interactive data (wakeup window, current alarm index, active server connection, etc.).
 * Should be called every time the wakeup window changes, alarm index or active server connection changes.
 */
void update_screen();

/**
 * Function to test if values set in hello_world_main.c are reflected in screen.c (DEPRECATED!!!)
 */
void set_test(int test_new);

/**
 * Function to test if values set in hello_world_main.c are reflected in screen.c (DEPRECATED!!!)
 */
int get_test();

/**
 * Updates values used to show wakeup window on screen.
 * (Is used right now, but is kind of unneccessary, this function was created at a time where shared_globals.h was not fully understood yet)
 */
void update_alarm_on_screen(TimeSimple alarm_start_new, TimeSimple alarm_end_new);

/**
 * Draws the highlights on the screen for setting the wakeup window. This way the user knows what hour / minute they are about to manipulate.
 */
void draw_highlight(uint8_t index);