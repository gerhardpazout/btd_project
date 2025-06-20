// datacapture.h
#ifndef DATACAPTURE_H
#define DATACAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

void print_csv_file(const char *path);

void datacapture_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // DATACAPTURE_H