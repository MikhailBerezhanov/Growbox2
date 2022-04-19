#ifndef _FLC01_H_ 
#define _FLC01_H_

#include "jsont.h"
	
typedef enum{
	current,
	start,
	stop
}flc_time_t;

int flc_init (const char* tty_name);

void flc_deinit(void);

int flc_get_status(flc_resp_t *status);

res_t flc_set_chpwr(flc_ch_pwr_t *ch_pwr);
res_t flc_set_time(flc_time_t type, my_time_t *tm);
res_t flc_set_smooth_ctrl(bool smooth);
res_t flc_set_smooth_value(int smooth_value);

#endif