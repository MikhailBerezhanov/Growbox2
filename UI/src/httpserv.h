#ifndef _HTTPSERV_H_ 
#define _HTTPSERV_H_

#include "fcgi_config.h" 
#include "fcgiapp.h"

#define MAX_POST_LEN		2048

#define URI_RFU				"/rfu"
#define URI_TEMPLATE		"/template"

#define URI_WELCOME			"/cncu_welcome.html"
#define URI_WELCOME_ERR		"/cncu_welcome_err.hmtl"
#define URI_LOGIN			"/login"
#define URI_MAIN			"/cncu_mainmenu.html"
#define URI_MAIN_CONTENT	"/content"
#define URI_SETTINGS		"/settings"
#define URI_CURR_SETTINGS	"/current_settings"
#define URI_RETURN_MAIN		"/return_mainmenu"
#define URI_LIGHT			"/light_settings"
#define URI_WATER			"/water_status"
#define URI_CLIMATE			"/climate_status"
#define URI_COOLERS			"/coolers_status"

#define URI_COOLERS_CURR	"/coolers/curr_state"
#define URI_COOLERS_POWER	"/coolers/power"
#define URI_COOLERS_ST_IN 	"/coolers/start_in_time"
#define URI_COOLERS_STP_IN 	"/coolers/stop_in_time"
#define URI_COOLERS_WP_IN 	"/coolers/wp_in_time"
#define URI_COOLERS_ST_OUT 	"/coolers/start_out_time"
#define URI_COOLERS_STP_OUT "/coolers/stop_out_time"
#define URI_COOLERS_WP_OUT 	"/coolers/wp_out_time"

#define URI_FLC_UPDATE		"/flc/update"
#define URI_FLC_CURR		"/flc/current_time"
#define URI_FLC_START		"/flc/start_time"
#define URI_FLC_STOP		"/flc/stop_time"
#define URI_FLC_SMOOTH		"/flc/smooth_control"
#define URI_FLC_SMOOTH_VAL	"/flc/smooth_value"
#define URI_FLC_CH_PWR		"/flc/ch_power"

#define URI_SETT_LAN		"/settings/lan"
#define URI_SETT_WIFIAP_ON	"/settings/wifi_ap_state?on"
#define URI_SETT_WIFIAP_OFF	"/settings/wifi_ap_state?off"
#define URI_SETT_RTC		"/settings/rtc"
#define URI_SETT_ACC		"/settings/acc"
#define URI_SETT_CPU_REBOOT	"/settings/cpu/reboot"
#define URI_SETT_CPU_PWROFF	"/settings/cpu/poweroff"
#define URI_SETT_CPU_DFLT	"/settings/cpu/restore_settings"

#define URI_SET_TH_PERIOD	"/climate/set_temp_hum_period"
#define URI_LATEST_TH		"/climate/latest_temp_hum"
#define URI_AVAIL_TH		"/climate/avail_dates"
#define URI_DAY_TH			"/climate/day_temp_hum"
#define URI_WEEK_TH			"/climate/week_temp_hum"
#define URI_MONTH_TH		"/climate/month_temp_hum"

void httpserv_serve (FCGX_Request *request);

#endif
