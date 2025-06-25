#ifndef DATACAPTURE_H
#define DATACAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Logs a given timestamp in a human readably format. 
 * Used for debugging purposes.
 */
void log_timestamp_readable(int64_t ts_ms);

/**
 * Prints the contents of a CSV file.
 * Used for debugging.
 */
void print_csv_file(const char *path);

/**
 * The task responsible for sampling data (accell & temp).
 */
void datacapture_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // DATACAPTURE_H