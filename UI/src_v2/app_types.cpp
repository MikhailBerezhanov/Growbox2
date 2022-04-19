#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "json.hpp"
#include "app_types.h"

#define LOG_MODULE_NAME   "APP_API"
#include "loger.h"


using json = nlohmann::json;

std::string result_to_json(const result& resp)
{
	json j;

	j["result_code"] = resp.code;
	j["result_text"] = resp.text;

	return j.dump();
}

s_time json_to_time(const std::string& json_str)
{
	auto j = json::parse(json_str);

	s_time t;
	t.hour = j["hour"];
	t.min = j["min"];
	t.sec = j["sec"];

	return t;
}


// ---------------------------------------------------------------------------------------------
std::string flc_req_to_json(const FLC_request& req)
{
	json j;

	if(!req.curr_time.empty()) j["current_time"] = req.curr_time;
	if(!req.start_time.empty()) j["start_time"] = req.start_time;
	if(!req.stop_time.empty()) j["stop_time"] = req.start_time;

	if(req.ch_pwr.ch1 >= 0) j["ch1_power"] = req.ch_pwr.ch1;
	if(req.ch_pwr.ch2 >= 0) j["ch2_power"] = req.ch_pwr.ch2;
	if(req.ch_pwr.ch3 >= 0) j["ch3_power"] = req.ch_pwr.ch1;

	if(req.smooth >= 0) j["smooth_control"] = req.smooth ? true : false;
	if(req.smooth_value >= 0) j["smooth_value"] = req.smooth_value;

	log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

FLC_response json_to_flc_resp(const std::string& json_str)
{
	auto j = json::parse(json_str);

	FLC_response resp;
	resp.res.code = j["result_code"];
	resp.res.text = j["result_text"];
	resp.curr_dtime = j["current_dtime"];
	resp.start_time = j["start_time"];
	resp.stop_time = j["stop_time"];
	resp.power_state = j["power_state"];
	resp.ch_pwr.ch1 = j["ch1_power"];
	resp.ch_pwr.ch2 = j["ch2_power"];
	resp.ch_pwr.ch3 = j["ch3_power"];
	resp.smooth = j["smooth_control"];
	resp.smooth_value = j["smooth_value"];

	log_msg(MSG_TRACE, _LGRAY _BOLD "%s()" _RESET ":\n", __func__);
	if(Log::check_lvl(MSG_TRACE)) std::cout << std::setw(4) << j << std::endl;
	
	return resp;
}

FLC::ch_pwr json_to_FLC_ch_pwr(const std::string& json_str)
{
	auto j = json::parse(json_str);

	FLC::ch_pwr p;
	p.ch1 = j["ch1_power"];
	p.ch2 = j["ch2_power"];
	p.ch3 = j["ch3_power"];

	return p;
}

bool json_to_flc_smooth(const std::string& json_str)
{
	auto j = json::parse(json_str);

	return j["smooth_control"];
}

int json_to_flc_smooth_value(const std::string& json_str)
{
	auto j = json::parse(json_str);

	return j["smooth_value"];
}
// ---------------------------------------------------------------------------------------------


std::string main_content_to_json(const main_content& resp)
{
	json j;

	j["temperature"] = resp.temperature;
	j["humidity"] = resp.humidity;
	j["water"] = resp.water;
	j["wind_in"] = resp.wind_in;
	j["wind_out"] = resp.wind_out;
	j["light"] = resp.light;

    log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

std::string curr_settings_to_json(const curr_settings& resp)
{
	json j;

	j["LAN"]["ip1"] = resp.lan.ip1;
	j["LAN"]["ip2"] = resp.lan.ip2;
	j["LAN"]["ip3"] = resp.lan.ip3;
	j["LAN"]["ip4"] = resp.lan.ip4;
	j["LAN"]["gw1"] = resp.lan.gw1;
	j["LAN"]["gw2"] = resp.lan.gw2;
	j["LAN"]["gw3"] = resp.lan.gw3;
	j["LAN"]["gw4"] = resp.lan.gw4;
	j["LAN"]["dhcp"] = resp.lan.dhcp_state;

	j["WIFI"]["ap_state"] = resp.wifi.ap_state;

	j["RTC"]["hour"] = resp.rtc.hour;
	j["RTC"]["min"] = resp.rtc.min;
	j["RTC"]["sec"] = resp.rtc.sec;
	j["RTC"]["mday"] = resp.rtc.mday;
	j["RTC"]["month"] = resp.rtc.month;
	j["RTC"]["year"] = resp.rtc.year;

	j["CPU"]["temperature"] = resp.cpu.temperature;
	j["CPU"]["app_ver"] = resp.cpu.app_version;
	
    log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

lan_set json_to_lan_settings(const std::string& json_str)
{
	auto j = json::parse(json_str);

	lan_set ls;
	ls.ip1 = j["ip1"];
	ls.ip2 = j["ip2"];
	ls.ip3 = j["ip3"];
	ls.ip4 = j["ip4"];
	ls.gw1 = j["gw1"];
	ls.gw2 = j["gw2"];
	ls.gw3 = j["gw3"];
	ls.gw4 = j["gw4"];
	ls.dhcp_state = j["dhcp"];

	return ls;
}

rtc_set json_to_rtc_settings(const std::string& json_str)
{
	auto j = json::parse(json_str);

	rtc_set rs;
	rs.hour = j["hour"];
	rs.min = j["min"];
	rs.sec = j["sec"];
	rs.mday = j["mday"];
	rs.month = j["month"];
	rs.year = j["year"];

	return rs;
}


std::string light_status_to_json(const light_status& resp)
{
	json j;
	/* TODO
	j["result"]["code"] = resp.res.code;
	j["result"]["text"] = resp.res.text;
	*/
	j["result_code"] = resp.res.code;
	j["result_text"] = resp.res.text;

	j["power_state"] = resp.power_state;
	j["curr_hour"] = resp.curr_time_hour;
	j["curr_min"] = resp.curr_time_min;
	j["curr_sec"] = resp.curr_time_sec;
	j["start_hour"] = resp.start_time_hour;
	j["start_min"] = resp.start_time_min;
	j["start_sec"] = resp.start_time_sec;
	j["stop_hour"] = resp.stop_time_hour;
	j["stop_min"] = resp.stop_time_min;
	j["stop_sec"] = resp.stop_time_sec;
	j["white_power"] = resp.ch_pwr.ch1;
	j["red_power"] = resp.ch_pwr.ch2;
	j["blue_power"] = resp.ch_pwr.ch3;
	j["smooth"] = resp.smooth;
	j["smooth_value"] = resp.smooth_value;

	log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

std::string coolers_state_to_json(const coolers_state& resp)
{
	json j;

	j["result"]["code"] = resp.res.code;
	j["result"]["text"] = resp.res.text;

	if(resp.arr.empty()) throw std::invalid_argument(excp_msg("coolers_state array is empty"));

	j["coolers_state"] = json::array();
	for(size_t i = 0; i < resp.arr.size(); ++i){
		json j_elem;
		j_elem["type"] = resp.arr[i].type;
		j_elem["power"] = resp.arr[i].power;
		j_elem["start_hour"] = resp.arr[i].start_hour;
		j_elem["start_min"] = resp.arr[i].start_min;
		j_elem["start_sec"] = resp.arr[i].start_sec;
		j_elem["stop_hour"] = resp.arr[i].stop_hour;
		j_elem["stop_min"] = resp.arr[i].stop_min;
		j_elem["stop_sec"] = resp.arr[i].stop_sec;
		j_elem["wp_min"] = resp.arr[i].wp_min;
		j_elem["is_on"] = resp.arr[i].is_on;
		j["coolers_state"].push_back(j_elem);
	}

	log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

coolers_pwr json_to_coolers_power(const std::string& json_str)
{
	auto j = json::parse(json_str);

	coolers_pwr cp;
	cp.in = j["wind_in"];
	cp.out = j["wind_out"];

	return cp;
}


int json_to_THS_period(const std::string& json_str)
{
	auto j = json::parse(json_str);

	return j["poll_period"];
}

THS_date json_to_THS_date(const std::string& json_str)
{
	auto j = json::parse(json_str);

	THS_date d;
	d.day = j["day"];
	d.month = j["month"];
	d.year = j["year"];

	return d; 
}

std::string THS_avail_dates_to_json(const THS_avail_dates& resp)
{
	json j;

	j["result"]["code"] = resp.res.code;
	j["result"]["text"] = resp.res.text;

	j["avail_dates"] = json::array();
	for(size_t i = 0; i < resp.dates.size(); ++i){
		json j_elem;
		j_elem["date"] = resp.dates[i];
		j["avail_dates"].push_back(j_elem);
	}

	log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}

std::string THS_response_to_json(const THS_response& resp)
{
	json j;

	j["result"]["code"] = resp.res.code;
	j["result"]["text"] = resp.res.text;

	j["poll_period"] = resp.poll_period;

	j["temp_hum_data"] = json::array();
	for(size_t i = 0; i < resp.arr.size(); ++i){
		json j_elem;
		j_elem["temp"] = resp.arr[i].temp;
		j_elem["hum"] = resp.arr[i].hum;
		j_elem["time"] = resp.arr[i].time;
		j["temp_hum_data"].push_back(j_elem);
	}

	log_msg(MSG_TRACE, _YELLOW _BOLD "%s()" _RESET ":\n%s\n", __func__, j.dump(4));
    return j.dump();
}


#ifdef UNIT_TEST

int main(int argc, char* argv[])
{
	Log::init(MSG_TRACE, "Log.log", 4096);

	try{
		main_content resp;
		resp.temperature = "123qwe";
		resp.humidity = "4545";
		resp.water = "ghgh";
		resp.wind_in = "OK";
		resp.wind_out = "qwerty";
		resp.light = "On";
		std::string ret = main_content_to_json(resp);
		log_msg(MSG_DEBUG, "main_content string:\n%s\n", ret);

		std::string str = "{\"result_code\":0,\"result_text\":\"OK\",\"current_dtime\":\"10.01.1980 21:25:53\",\"power_state\":\"On\",\"ch1_power\":10,\"ch2_power\":10,\"ch3_power\":10,\"start_time\":\"10:00:00\",\"stop_time\":\"22:00:00\",\"smooth_control\":true,\"smooth_value\":-1}";
		FLC_response fresp = json_to_flc_resp(str);

		curr_settings cs;
		curr_settings_to_json(cs);

		FLC_request freq;
		flc_req_to_json(freq);

		cooler_status cl_st[2] = { 
			cooler_status("in", 50, "12", "00", "00", "22", "30", "50", "45", false),
			cooler_status("out", 43, "13", "10", "20", "22", "00", "00", "15", true),
		};

		coolers_state cl_state;
		cl_state.arr.push_back(cl_st[0]);
		cl_state.arr.push_back(cl_st[1]);
		coolers_state_to_json(cl_state);

	}
	catch(const std::exception& e){	
		log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
	}



	return 0;
}

#endif