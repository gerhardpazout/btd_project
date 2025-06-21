#ifndef DATACAPTURE_H
#define DATACAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int64_t now_ms(void);

void log_timestamp_readable(int64_t ts_ms);

void print_csv_file(const char *path);

void datacapture_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // DATACAPTURE_H