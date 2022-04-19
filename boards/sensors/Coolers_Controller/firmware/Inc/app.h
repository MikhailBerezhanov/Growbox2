#ifndef _APP_H
#define _APP_H

#include <stdint.h>
#include "rtc.h"

#define WORK_PERIODS_NUM		5		// Supported number of work periods to serve

// Smooth power-control settings
#define TIM2_PERIOD					( (uint16_t)2000 )		// [ * 0.1 ms ] (MAX 65356)
#define PWM_DISCRETE				1		// [%]

#define RTC_POLL_PERIOD			2		// [sec] peroid for current time polling

typedef enum{
	APP_OK = 0,							
	APP_INVALID_DTIME_FORMAT,
	APP_INVALID_TIME_FORMAT,
	APP_CRITICAL_ERROR,
	APP_JSON_PARSE_ERR,
	APP_JSON_NO_OBJ,
	APP_INVALID_DATA,	
	APP_DATA_OVERFLOW,
}app_err_t;

// This data is stored in internal FLASH, so we use words (32bits) 
typedef __packed struct {
  int32_t start_hour;
	int32_t start_min;
	int32_t start_sec;
	int32_t stop_hour;
	int32_t stop_min;
	int32_t stop_sec;
	int32_t enabled;
}work_period_t;

typedef __packed struct{
	int32_t r;			// Red leds PWM %
	int32_t b;    	// Blue leds PWM %
	int32_t w;    	// White leds PWM %
}pwm_val_t;

// Request (New settings)
typedef struct{
	pwm_val_t pwm;
	char smooth_control[8];
	char curr_dtime[32];
	char curr_time[16];
	char start_time[16];
	char stop_time[16];
}req_t;

typedef __packed struct{
	uint32_t smooth_control;
	pwm_val_t pwm;
	work_period_t wp[WORK_PERIODS_NUM];
}app_data_t;


void app_init(void);
void serve_input(void);
void check_work_periods(rt_t *rt);
void smooth_power_up_handler(void);
void adjust_led_power(rt_t *rt);

#endif
