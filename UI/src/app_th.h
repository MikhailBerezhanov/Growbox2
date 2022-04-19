#ifndef _APP_TEMP_HUM_H_ 
#define _APP_TEMP_HUM_H_

#include <stdint.h>
#include <stdbool.h>
#include "app.h"

int app_th_init(mydb* db);
void app_th_wakeup(void);
void app_th_deinit(void);

#endif