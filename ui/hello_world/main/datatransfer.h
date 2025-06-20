#ifndef DATATRANSFER_H
#define DATATRANSFER_H

#include <stdbool.h>

// Starts the data transfer FreeRTOS task (definition in data_transfer_task.c)
void data_transfer_task(void *pvParameters);

#endif
