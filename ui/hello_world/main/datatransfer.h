#ifndef DATATRANSFER_H
#define DATATRANSFER_H

#include <stdbool.h>

/**
 * Task responsible for data transfer between ESP and server.
 * Also triggers alarm.
 */
void data_transfer_task(void *pvParameters);

/**
 * Sends the wakeup window set by user.
 * Called in data_transfer_task()
 */
void send_wakeup_timewindow();

#endif
