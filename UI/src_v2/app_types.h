#ifndef _APP_TYPES_H
#define _APP_TYPES_H

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#include "flc.h"

//--------------------------------- [ UI Common ] -----------------------------------

struct result
{
	result(): code(-1), text("undefined") { }
	int code;
	std::string text;
};

struct s_time
{
	s_time(uint h = 0, uint m = 0, uint s = 0): hour(h), min(m), sec(s) {}
	uint hour = 0;
	uint min = 0;
	uint sec = 0;
};

//-------------------------------- [ FLC PROTOCOL ] ---------------------------------

// FLC - Fito Lamp Controller

struct FLC_request
{
	std::string curr_time;
	std::string start_time;
	std::string stop_time;
	FLC::ch_pwr ch_pwr;
	int smooth = -1;		// -1 - no value
	int smooth_value = -1;	// -1 - no value
};

struct FLC_response
{
	result res;
	std::string power_state;
	std::string curr_dtime;
	std::string start_time;
	std::string stop_time;
	bool smooth = false;
	int smooth_value = 0;
	FLC::ch_pwr ch_pwr;
};

//---------------------------------- [ MAIN MENU ] ----------------------------------

struct main_content
{
	std::string temperature;
	std::string humidity;
	std::string water;
	std::string light;
	std::string wind_in;
	std::string wind_out;
};


//----------------------------------- [ SETINGS ] -----------------------------------

struct lan_set
{
	std::string ip1;
	std::string ip2;
	std::string ip3;
	std::string ip4;
	std::string gw1;
	std::string gw2;
	std::string gw3;
	std::string gw4;
	bool dhcp_state = false;
};

struct wifi_set
{
	bool ap_state = false;
};

struct rtc_set
{ 
	std::string hour;
	std::string min;
	std::string sec;
	std::string mday;
	std::string month;
	std::string year;
};

struct cpu_set
{
	std::string temperature;
	std::string load_percent;
	std::string app_version;
};

struct curr_settings
{
	lan_set lan;
	wifi_set wifi;
	rtc_set rtc;
	// TODO: acc_set acc;
	cpu_set cpu;	
};


//--------------------------------- [ LIGHT ] ----------------------------------

struct light_status
{
	result res;
	bool power_state = false;
	std::string curr_time_hour;
	std::string curr_time_min;
	std::string curr_time_sec;
	std::string start_time_hour;
	std::string start_time_min;
	std::string start_time_sec;
	std::string stop_time_hour;
	std::string stop_time_min;
	std::string stop_time_sec;
	bool smooth = false;
	int smooth_value = -1;
	FLC::ch_pwr ch_pwr;
};


//---------------------------------- [ WIND ] ----------------------------------

struct coolers_pwr
{
	int in = -1;
	int out = -1;
};

struct cooler_status
{
	cooler_status() = default;
	cooler_status(
		const std::string& t, 
		int p, 
		const std::string& sh, 
		const std::string& sm, 
		const std::string& ss, 
		const std::string& ssh, 
		const std::string& ssm, 
		const std::string& sss,
		const std::string& wm, 
		bool on): 
	type(t), power(p), start_hour(sh), start_min(sm), start_sec(ss),
	stop_hour(ssh), stop_min(ssm), stop_sec(sss), wp_min(wm), is_on(on) {}

	std::string type;
	int power = -1;
	std::string start_hour;
	std::string start_min;
	std::string start_sec;
	std::string stop_hour;
	std::string stop_min;
	std::string stop_sec;
	std::string wp_min;
	bool is_on = false;
};

struct coolers_state
{
	result res;
	std::vector<cooler_status> arr;
};


//---------------------------------- [ CLIMATE ] ----------------------------------

// THS  - temperature humidity sensor

struct THS_data
{
	std::string time;
	float temp = 0.0;
	float hum = 0.0;
};

struct THS_response
{
	result res;
	int poll_period = -1;
	std::vector<THS_data> arr;
};

struct THS_avail_dates
{
	result res;
	std::vector<std::string> dates;
};

struct THS_date
{
	uint32_t day = 0;
	uint32_t month = 0;
	uint32_t year = 0;
};


#endif