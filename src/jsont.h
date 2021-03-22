#ifndef _JSONT_H_ 
#define _JSONT_H_

#include <stdint.h>
#include <string.h>

typedef enum{
	J_OK = 0,
	J_INVALID_FORMAT,
	J_FIELD_MISSING,
	J_UNKNOWN_ERR,
	J_PARSER_ERR,
	J_OUT_OF_RANGE,
	J_TOTAL_ERR_NUMBER
}jerr_t;

const char* jsont_get_err_text(jerr_t err);

//---------------------------------- [ UI Common ] ----------------------------------
typedef struct flc_res{
	flc_res() {
		this->code = -1;
		strcpy(this->text, "undefined");
	};
	int code;
	char text[128];
}res_t;

typedef struct{
	uint hour;
	uint min;
	uint sec;
}my_time_t;

// NOTE: Don't forget to free() ouput c-string JSON
char* res_to_json(res_t *resp);

//---------------------------------- [ MAIN MENU ] ----------------------------------

typedef struct content_resp{
	content_resp() { memset(this, 0, sizeof(content_resp)); };
	char temperature[32];
	char humidity[32];
	char water[32];
	char light[32];
	char wind_in[32];
	char wind_out[32];
}content_resp_t;

char* content_resp_to_json(content_resp_t *resp);


//---------------------------------- [ SETINGS ] ----------------------------------

typedef struct lan_sett{
	lan_sett() { memset(this, 0, sizeof(lan_sett)); };
	char ip4[4];
	char ip3[4];
	char ip2[4];
	char ip1[4];
	char gw4[4];
	char gw3[4];
	char gw2[4];
	char gw1[4];
	bool dhcp;
}lan_sett_t;

typedef struct wifi_sett{
	wifi_sett() { memset(this, 0, sizeof(wifi_sett)); };
	bool ap_state;
}wifi_sett_t;

typedef struct rtc_sett{ 
	rtc_sett() { memset(this, '\0', sizeof(rtc_sett)); };
	char hour[4];
	char min[4];
	char sec[4];
	char mday[4];
	char month[4];
	char year[8];
}rtc_sett_t;

typedef struct cpu_sett{
	cpu_sett() { memset(this, 0, sizeof(cpu_sett)); };
	char temperature[8];
	char load_percent[8];
	char app_version[8];
}cpu_sett_t;

typedef struct curr_sett{
	curr_sett() { memset(this,0,sizeof(curr_sett)); };
	lan_sett_t lan;
	wifi_sett_t wifi;
	rtc_sett_t rtc;
	// ACC
	cpu_sett_t cpu;	
}curr_sett_t;

char* curr_sett_resp_to_json(curr_sett_t *resp);
jerr_t json_to_lan_settings(const char *json, lan_sett_t* req);
jerr_t json_to_rtc_settings(const char *json, rtc_sett_t* req);



//---------------------------------- [ FLC PROTOCOL ] ----------------------------------

typedef struct flc_ch_pwr{
	flc_ch_pwr() { memset(this, -1, sizeof(flc_ch_pwr)); };
	int ch1;
	int ch2;
	int ch3;
}flc_ch_pwr_t;

typedef struct flc_req{
	flc_req() {
		memset(this, -1, sizeof(flc_req));
		strcpy(this->curr_time, "");
		strcpy(this->start_time, "");
		strcpy(this->stop_time, "");
	};
	char curr_time[32];
	char start_time[32];
	char stop_time[32];
	flc_ch_pwr_t ch_pwr;
	int smooth;			// -1 - no value
	int smooth_value;	// -1 - no value
}flc_req_t;

typedef struct flc_resp{
	flc_resp() { memset(this, 0, sizeof(flc_resp)); };
	res_t res;
	char power_state[32];
	char curr_dtime[32];
	char start_time[32];
	char stop_time[32];
	bool smooth;
	int smooth_value;
	flc_ch_pwr_t ch_pwr;
}flc_resp_t;

typedef struct flc_status{
	flc_status() { memset(this, 0, sizeof(flc_status)); };
	res_t res;
	bool power_state;
	char curr_time_hour[4];
	char curr_time_min[4];
	char curr_time_sec[4];
	char start_time_hour[4];
	char start_time_min[4];
	char start_time_sec[4];
	char stop_time_hour[4];
	char stop_time_min[4];
	char stop_time_sec[4];
	bool smooth;
	int smooth_value;
	flc_ch_pwr_t ch_pwr;
}flc_status_t;

// TO, FROM FLC
char* flc_req_to_json(flc_req_t *req);
jerr_t json_to_flc_resp(const char *json, flc_resp_t *resp);

// TO, FROM UI (AJAX)
jerr_t json_to_flc_ch_pwr(const char *json, flc_ch_pwr_t *req);
jerr_t json_to_my_time(const char *json, my_time_t *req);
jerr_t json_to_flc_smooth(const char *json, bool *req);
jerr_t json_to_flc_smooth_value(const char *json, int *req);
char* flc_status_to_json(flc_status_t *resp);

//---------------------------------- [ WIND ] ----------------------------------

typedef struct coolers_pwr{
	coolers_pwr() { memset(this, -1, sizeof(coolers_pwr)); };
	int in;
	int out;
}coolers_pwr_t;

typedef struct cooler_status{
	cooler_status() { memset(this, 0, sizeof(cooler_status)); };
	char type[16];
	int power;
	char start_hour[4];
	char start_min[4];
	char start_sec[4];
	char stop_hour[4];
	char stop_min[4];
	char stop_sec[4];
	char wp_min[4];
	bool is_on;
}cooler_status_t;

typedef struct coolers_state{
	coolers_state() { memset(this, 0, sizeof(coolers_state)); };
	res_t res;
	cooler_status_t* arr;
	uint arr_size;
}coolers_state_t;

jerr_t json_to_coolers_power(const char *json, coolers_pwr_t* req);
char* coolers_state_to_json(coolers_state_t* resp);

//---------------------------------- [ CLIMATE ] ----------------------------------

typedef struct th_data{
	th_data() { memset(this, 0, sizeof(th_data)); };
	char time[32];
	float temp;
	float hum;
}th_data_t;

typedef struct th_data_resp{
	th_data_resp() { memset(this, 0, sizeof(th_data_resp)); this->poll_period = -1;}
	res_t res;
	int poll_period;
	th_data_t *arr;
	size_t arr_size;
}th_data_resp_t;

typedef struct th_adate{
	th_adate() { memset(this, 0, sizeof(th_adate)); };
	char date[16];
}th_adate_t;

typedef struct th_avail_resp{
	th_avail_resp() { memset(this, 0, sizeof(th_avail_resp));}
	res_t res;
	th_adate_t *arr;
	size_t arr_size;
}th_avail_resp_t;


typedef struct th_date_req{
	th_date_req() { memset(this, 0, sizeof(th_date_req));}
	uint32_t day;
	uint32_t month;
	uint32_t year;
}th_date_req_t;

jerr_t json_to_th_period(const char* json, int* period_min);
char* th_data_to_json(th_data_resp_t* resp);
char* th_avail_to_json(th_avail_resp_t* resp);
jerr_t json_to_th_date(const char* json, th_date_req_t* req);

#endif