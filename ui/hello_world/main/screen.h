#pragma once
#include "time_simple.h"

void list_spiffs_files();

void init_spiffs(void);

void init_screen();

void render_screen();

void update_screen();

void set_test(int test_new);

int get_test();

void update_alarm_on_screen(TimeSimple alarm_start_new, TimeSimple alarm_end_new);

void draw_highlight(uint8_t index);