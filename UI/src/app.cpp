#include <pthread.h> 
#include <libconfig.h>
#include <linux/limits.h>
#include <unistd.h>
#include <inttypes.h>

#include <cstdlib>
#include <ctime>

#include "utils.h"
#include "jsont.h"
#include "flc.h"
#include "sensors.h"
#include "config.h"
#include "app_cl.h"
#include "app_th.h"
#include "app.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ APP ] "
#include "dbgmsg.h"

using namespace std;

pthread_mutex_t app_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t app_coolers_mutex = PTHREAD_MUTEX_INITIALIZER;
app_data_t app_data;

static mydb db;

static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t flc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread to work with temperature-humidity sensor
static pthread_t temp_hum_worker_id;
static void* temp_hum_worker(void *arg);


static void init_config_data(void)
{
	memset(&app_data.conf, 0, sizeof(app_data.conf));

	strcat(app_data.conf.startup_page, app_data.cwd);
	strcat(app_data.conf.login_err_page, app_data.cwd);
	strcat(app_data.conf.mainmenu_page, app_data.cwd);
	strcat(app_data.conf._404_page, app_data.cwd);
	strcat(app_data.conf.settings_page, app_data.cwd);
	strcat(app_data.conf.flc_page, app_data.cwd);
	strcat(app_data.conf.water_page, app_data.cwd);
	strcat(app_data.conf.temperature_page, app_data.cwd);
	strcat(app_data.conf.cooler_page, app_data.cwd);
	strcat(app_data.conf.template_page, app_data.cwd);
}

int app_read_config(void)
{
  pthread_mutex_lock(&app_data_mutex);

  int intval = 0;

  init_config_data();

	if( conf_init(app_data.config_path) < 0){
  	pthread_mutex_unlock(&app_data_mutex);
    return -1;
  }
  
  conf_bool("test_mode", &app_data.conf.test_mode, false);
  conf_int("log_level", &app_data.conf.log_level, MSG_VERBOSE, "[level]");
  conf_int("log_max_size", &intval, DFL_LOG_MAX_SIZE, "[MB]");
  app_data.conf.log_max_size = (uint64_t)intval * 1024 * 1024;
  dbgmsg_v(MSG_TRACE, "log_max_size: %" PRIu64 " [bytes]\n", app_data.conf.log_max_size);
  conf_int("mydb_max_size", &intval, DFL_LOG_MAX_SIZE, "[MB]");
  app_data.conf.mydb_max_size = (uint64_t)intval * 1024 * 1024;
  dbgmsg_v(MSG_TRACE, "mydb_max_size: %" PRIu64 " [bytes]\n", app_data.conf.mydb_max_size);

  conf_int("login_timeout", &app_data.conf.login_timeout, DFLT_LOGIN_TMOUT, "[sec]");
  conf_int("TH_poll_period", &intval, 0, "[min]");
  app_data.conf.th_poll_period = (uint64_t)intval * 60;
  dbgmsg_v(MSG_TRACE | MSG_TO_FILE, "th_poll_period: %" PRIu64 " [sec]\n", app_data.conf.th_poll_period);
  conf_int("cl_poll_period", &app_data.conf.cl_poll_period, 60, "[sec]");

  conf_str("lan_iface_name", app_data.conf.lan_iface_name, "enp0s3");
  conf_str("lan_connection_name", app_data.conf.lan_conn_name, "Проводное соединение 1");
  conf_str("wifi_ap_script_path", app_data.conf.wifi_ap_script_path, "/opt/growbox/start_wifi_ap.sh");

	conf_str("startup_page", app_data.conf.startup_page, "");
	conf_str("mainmenu_page", app_data.conf.mainmenu_page, "");
	conf_str("page_404", app_data.conf._404_page, "");
	conf_str("template_page", app_data.conf.template_page, "");
	conf_str("settings_page", app_data.conf.settings_page, "");
	conf_str("flc_page", app_data.conf.flc_page, "");
	conf_str("water_page", app_data.conf.water_page, "");
	conf_str("temperature_page", app_data.conf.temperature_page, "");
	conf_str("cooler_page", app_data.conf.cooler_page, "");
	conf_str("login_err_page", app_data.conf.login_err_page, "");
	conf_str("username", app_data.conf.username, "");
	conf_str("password", app_data.conf.password, "");
	conf_str("flc_tty", app_data.conf.flc_tty, "/dev/ttyS4");
	conf_str("i2c_path", app_data.conf.i2c_path, "/dev/i2c-0");
	conf_str("cpu_temp_path", app_data.conf.cpu_temp_path, "/sys/class/thermal/thermal_zone0/temp");

	conf_bool("remote_reboot_enabled", &app_data.conf.remote_reboot, false);
	conf_bool("remote_poweroff_enabled", &app_data.conf.remote_poweroff, false);

	pthread_mutex_unlock(&app_data_mutex);

	conf_deinit();
	return 0;
}

int app_init(void)
{
	memset(&app_data, 0, sizeof(app_data_t));

	if (get_cwd(app_data.cwd, sizeof(app_data.cwd)) <0 )
		strcpy(app_data.cwd, DFLT_CWD);

	dbgmsg_v(MSG_DEBUG, "~ Starting GBWEB version: '%s', build-time: '%s %s' ~\n", APP_VERSION, __DATE__, __TIME__);
	dbgmsg_v(MSG_DEBUG, "*** Working directory: '%s'\n", app_data.cwd);

	strcat(app_data.config_path, app_data.cwd);
	strcat(app_data.config_path, "/gbweb.conf");
	strcat(app_data.log_path, app_data.cwd);
	strcat(app_data.log_path, "/gbweb.log");
	strcat(app_data.cgi_sock_path, app_data.cwd);
	strcat(app_data.cgi_sock_path, "/gbweb.sock");
	strcat(app_data.data_dir, app_data.cwd);
	strcat(app_data.data_dir, "/app_data");
	strcat(app_data.db_dir, app_data.cwd);
	strcat(app_data.db_dir, "/my.db");

	log_init(app_data.log_path, 0);
	app_read_config();

	dbgmsg_v(MSG_TO_FILE, "Setting log level to : %d\n", app_data.conf.log_level);
	set_log_level(app_data.conf.log_level);
	make_new_dir(app_data.data_dir);

	flc_init(app_data.conf.flc_tty);
	sensors_init(app_data.conf.i2c_path);

	if (db.init(app_data.db_dir, CREATE | READWRITE | FULLMUTEX) != MYDB_OK) return -1;

	// Restore systemconfig data from database
	db.get_cl_time(&app_data.cwt);
	db.get_cl_power(input, &app_data.cl_in_power);
	db.get_cl_power(output, &app_data.cl_out_power);

	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Input cooler work schedule\n");
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Start:\t\t%02u:%02u\n", app_data.cwt.start_in.hour, app_data.cwt.start_in.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Stop:\t\t%02u:%02u\n", app_data.cwt.stop_in.hour, app_data.cwt.stop_in.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Work period:\t%u min\n", app_data.cwt.wp_in.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Power:\t\t%u\n", (uint)app_data.cl_in_power);

	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Output cooler work schedule\n");
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Start:\t\t%02u:%02u\n", app_data.cwt.start_out.hour, app_data.cwt.start_out.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Stop:\t\t%02u:%02u\n", app_data.cwt.stop_out.hour, app_data.cwt.stop_out.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Work period:\t%u min\n", app_data.cwt.wp_out.min);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Power:\t\t%u\n", (uint)app_data.cl_out_power);

	if(app_cl_init() < 0) return -1;

	if(app_th_init(&db) < 0) return -1;

	return 0;
}

void app_deinit(void)
{
	db.deinit();
	sensors_deinit();
	flc_deinit();
	app_cl_deinit();
	app_th_deinit();
}


static void* temp_hum_worker(void* arg)
{
	// Enable thread cancelling
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	float temperature = 0;
	float humidity = 0;
	int ret = SENS_OK;

	// period from config file has more priority
	pthread_mutex_lock(&app_data_mutex);
	uint64_t period = app_data.conf.th_poll_period;
	pthread_mutex_unlock(&app_data_mutex);

	if(!period) {
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "No TH period is set (%" PRIu64 ") in config. Reading from db..\n", period);
		
		if(db.get_th_period(&period) == MYDB_OK){
			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "TH period is %" PRIu64 " [sec]\n", period);
			app_data.conf.th_poll_period = period;
			if(!period){
				dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "[ INFO ] No TH period found. Cancelling TH worker thread..\n");
				return NULL;
			}
		}
	}

	for (;;) 
 	{
 		db.check_size(app_data.conf.mydb_max_size);
		// TEST MODE
		if(app_data.conf.test_mode){   
				srand ( time(NULL) );    
		    temperature = 2.25 * (float)(rand() % 20 + 1);
		    humidity = 1.25 * (float)(rand() % 80);
		    dbgmsg_v(MSG_VERBOSE, "[ TEST MODE ] Saving Temp: %f Hum: %f\n", temperature, humidity);
		}
		// WORK MODE
		else{
			// Get temperature and humidity from sensor
		  ret = sht30_read_data(&temperature, &humidity);
		}

		if(ret == SENS_OK) db.put_th_val(temperature, humidity);
		else errmsg_v(MSG_DEBUG | MSG_TO_FILE, "sht30_read_data() failed: %d\n", ret);

		// Renew poll period 
		pthread_mutex_lock(&app_data_mutex);
		period = app_data.conf.th_poll_period;
		pthread_mutex_unlock(&app_data_mutex);

		sleep(period);
	}

	return NULL;
}






char* app_get_sensors_data(int flags)
{
	int res = 0;
	content_resp_t sens;
	flc_resp_t flc_status;
	char fname[256] = {0};
	strcat(fname, app_data.data_dir);
	strcat(fname, "/ws_value.txt");
	uint ws_value = 0;
	float temperature = 27.789;
	float humidity = 80.12345;
	uint8_t cooler_pwr = 0;

	if(flags & GET_WATER)
	{
		if(get_water_sensor(fname, &ws_value) == SENS_OK){
			ws_value ? strcpy(sens.water, "OK") : strcpy(sens.water, "Empty");
		}
	}

	if(flags & GET_TEMP_HUM){
		if(sht30_read_data(&temperature, &humidity) == SENS_OK){
			sprintf(sens.temperature, "%.2f", temperature);
			sprintf(sens.humidity, "%.2f RH", humidity);
		}
	}

	if(flags & GET_COOLERS)
	{
		if(get_coolers_power(input, &cooler_pwr) == SENS_OK){
			sprintf(sens.wind_in, "input: %u %%", cooler_pwr);
		}
		if(get_coolers_power(output,  &cooler_pwr) == SENS_OK){
			sprintf(sens.wind_out, "output: %u %%", cooler_pwr);
		}
	}

	if(flags & GET_LIGHT){
		pthread_mutex_lock(&flc_mutex);
		res = flc_get_status(&flc_status);
		pthread_mutex_unlock(&flc_mutex);	
		if(!res && !strcmp(flc_status.res.text, "OK")){
			strcpy(sens.light, flc_status.power_state);
		}
	}

	// TEST MODE
	if(app_data.conf.test_mode){
		strcpy(sens.light, "On");
		strcpy(sens.water, "No");
		sprintf(sens.temperature, "%.2f", 25.76969);
		sprintf(sens.humidity, "%.2f RH", 80.4389898);
		sprintf(sens.wind_in, "input: %u %%", 23);
		sprintf(sens.wind_out, "output: %u %%", 21);
	}

	return content_resp_to_json(&sens);
}

char* app_get_curr_settings(void)
{
	static bool wifi_ap_on = false;		// for test mode only
	curr_sett_t settings;
	ipv4_t my_ip;
	ipv4_t my_gw;
	char fname[256] = {0};

	strcat(fname, app_data.data_dir);
	strcat(fname, "/my_ip.txt");

	pthread_mutex_lock(&settings_mutex);

// LAN
	if(get_current_ip(fname, &my_ip) < 0){	
		strcpy(settings.lan.ip4, "-");
		strcpy(settings.lan.ip3, "-");
		strcpy(settings.lan.ip2, "-");
		strcpy(settings.lan.ip1, "-");}
	else{
		strcpy(settings.lan.ip4, my_ip.ip4_as_ch);
		strcpy(settings.lan.ip3, my_ip.ip3_as_ch);
		strcpy(settings.lan.ip2, my_ip.ip2_as_ch);
		strcpy(settings.lan.ip1, my_ip.ip1_as_ch);
	}

	memset(fname, 0, sizeof(fname));
	strcat(fname, app_data.data_dir);
	strcat(fname, "/my_gw.txt");
	if(get_current_gw(fname, app_data.conf.lan_iface_name, &my_ip) < 0){	
		strcpy(settings.lan.gw4, "-");
		strcpy(settings.lan.gw3, "-");
		strcpy(settings.lan.gw2, "-");
		strcpy(settings.lan.gw1, "-");}
	else{
		strcpy(settings.lan.gw4, my_ip.ip4_as_ch);
		strcpy(settings.lan.gw3, my_ip.ip3_as_ch);
		strcpy(settings.lan.gw2, my_ip.ip2_as_ch);
		strcpy(settings.lan.gw1, my_ip.ip1_as_ch);
	}

	memset(fname, 0, sizeof(fname));
	strcat(fname, app_data.data_dir);
	strcat(fname, "/my_dhcp_state.txt");
	if(get_current_dhcp_state(fname, app_data.conf.lan_conn_name, &settings.lan.dhcp) < 0) {
		errmsg_v(MSG_DEBUG, "get_current_dhcp_state() failed\n");
	}

// WiFi
	memset(fname, 0, sizeof(fname));
	strcat(fname, app_data.data_dir);
	strcat(fname, "/my_wifi_ap_state.txt");
	if(app_data.conf.test_mode){
		settings.wifi.ap_state = wifi_ap_on;
		wifi_ap_on = !wifi_ap_on;
	}
	else{
		get_wifi_ap_state(fname, &settings.wifi.ap_state);
	}

// RTC
    struct tm tm;
    
    if(get_local_time(&tm)){
    	sprintf(settings.rtc.hour, "%02u", tm.tm_hour);
    	sprintf(settings.rtc.min, "%02u", tm.tm_min);
    	sprintf(settings.rtc.sec, "%02u", tm.tm_sec);
    	sprintf(settings.rtc.month, "%02u", tm.tm_mon);
    	sprintf(settings.rtc.mday, "%02u", tm.tm_mday);
    	sprintf(settings.rtc.year, "%04u", tm.tm_year);
    }
    else{
    	strcpy(settings.rtc.hour, "-");
    	strcpy(settings.rtc.min, "-");
    	strcpy(settings.rtc.sec, "-");
    	strcpy(settings.rtc.month, "-");
    	strcpy(settings.rtc.mday, "-");
    	strcpy(settings.rtc.year, "-");
    }

// ACC

// CPU
    strcpy(settings.cpu.app_version, APP_VERSION);
    if(get_cpu_temp(app_data.conf.cpu_temp_path, settings.cpu.temperature) < 0) strcpy(settings.cpu.temperature, "--");

	pthread_mutex_unlock(&settings_mutex);

	return curr_sett_resp_to_json(&settings);
}



static my_time_t flctime_to_mytime(char* flc_time)
{
	my_time_t my_time;
	char *time_ptr = flc_time;

	if (strchr(flc_time, '.'))	// curr_dtime
	{
		char *dtime_str = strtok(flc_time, " ");
		if(dtime_str) time_ptr += strlen(dtime_str) + 1;
	}

	char* istr = strtok(time_ptr, ":");
	if(istr) my_time.hour = atoi(istr);

	istr = strtok(NULL, ":");
	if(istr) my_time.min = atoi(istr);

	istr = strtok(NULL, ":");
	if(istr) my_time.sec = atoi(istr);

	//dbgmsg("my_time hour: %d, min: %d, sec: %d\n", my_time.hour, my_time.min, my_time.sec);

	return my_time;
}


static char* create_test_flc_status(void)
{
	flc_status_t fs;

	strcpy(fs.res.text, "OK");
	fs.res.code = 0;
	fs.power_state = true;
	fs.smooth = false;
	fs.smooth_value = 666;
	sprintf(fs.curr_time_hour, "%02u", 12);  
	sprintf(fs.curr_time_min, "%02u", 13);
	sprintf(fs.curr_time_sec, "%02u", 45);

	sprintf(fs.start_time_hour, "%02u", 8);
	sprintf(fs.start_time_min, "%02u", 0);
	sprintf(fs.start_time_sec, "%02u", 0);

	sprintf(fs.stop_time_hour, "%02u", 22);
	sprintf(fs.stop_time_min, "%02u", 35);
	sprintf(fs.stop_time_sec, "%02u", 00);

	fs.ch_pwr.ch1 = 43;
	fs.ch_pwr.ch2 = 99;
	fs.ch_pwr.ch3 = 60;

	return flc_status_to_json(&fs);
}

char* app_get_flc_status(void)
{
	flc_status_t fs;
	flc_resp_t flc_status;
	my_time_t my_time;
	int res = 0;

	if(app_data.conf.test_mode) return create_test_flc_status();

	pthread_mutex_lock(&flc_mutex);
	res = flc_get_status(&flc_status);
	pthread_mutex_unlock(&flc_mutex);	
	if (res < 0) {
		res_t flc_res;
		flc_res.code = res;
		switch(flc_res.code){
			case -1: strcpy(flc_res.text, "flc_send_request failed"); break;
			case -2: strcpy(flc_res.text, "flc_get_responce failed"); break;
			default: strcpy(flc_res.text, jsont_get_err_text((jerr_t)res)); break;
		}
		return(res_to_json(&flc_res));
	}	
	
	if(strcmp(flc_status.res.text, "OK")) return NULL;

	// Convert to AJAX json
	strcpy(fs.res.text, flc_status.res.text);
	fs.res.code = flc_status.res.code;

	if(!strcmp(flc_status.power_state, "On")) fs.power_state = true;
	else fs.power_state = false;

	fs.smooth = flc_status.smooth;
	fs.smooth_value = flc_status.smooth_value;

	my_time = flctime_to_mytime(flc_status.curr_dtime);
	sprintf(fs.curr_time_hour, "%02u", my_time.hour);  
	sprintf(fs.curr_time_min, "%02u", my_time.min);
	sprintf(fs.curr_time_sec, "%02u", my_time.sec);

	my_time = flctime_to_mytime(flc_status.start_time);
	sprintf(fs.start_time_hour, "%02u", my_time.hour);
	sprintf(fs.start_time_min, "%02u", my_time.min);
	sprintf(fs.start_time_sec, "%02u", my_time.sec);

	my_time = flctime_to_mytime(flc_status.stop_time);
	sprintf(fs.stop_time_hour, "%02u", my_time.hour);
	sprintf(fs.stop_time_min, "%02u", my_time.min);
	sprintf(fs.stop_time_sec, "%02u", my_time.sec);

	fs.ch_pwr.ch1 = flc_status.ch_pwr.ch1;
	fs.ch_pwr.ch2 = flc_status.ch_pwr.ch2;
	fs.ch_pwr.ch3 = flc_status.ch_pwr.ch3;

	return flc_status_to_json(&fs);
}

char* app_set_flc_ch_pwr(const char* json)
{
	jerr_t res;
	flc_ch_pwr_t ch_pwr;
	res_t flc_res;
	
	pthread_mutex_lock(&flc_mutex);
	if((res = json_to_flc_ch_pwr(json, &ch_pwr)) != J_OK){
		strcpy(flc_res.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, flc_res.text);
		flc_res.code = -2;
		return (res_to_json(&flc_res));
	} 

	// Send new settings to FLC
	flc_res = flc_set_chpwr(&ch_pwr);
	pthread_mutex_unlock(&flc_mutex);

	return (res_to_json(&flc_res));
}


char* app_set_flc_time(flc_time_t type, const char* json)
{
	jerr_t res;
	my_time_t tm;
	res_t result;

	if(app_data.conf.test_mode){
		result.code = 0;
		strcpy(result.text, "OK");
		return (res_to_json(&result));
	}

	if((res = json_to_my_time(json, &tm)) != J_OK){
		strcpy(result.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, result.text);
		result.code = -2;
		return (res_to_json(&result));
	} 

	pthread_mutex_lock(&flc_mutex);
	result = flc_set_time(type, &tm);
	pthread_mutex_unlock(&flc_mutex);

	return (res_to_json(&result));
}


char* app_set_flc_smooth(const char* json)
{
	jerr_t res;
	bool smooth;
	res_t result;

	if(app_data.conf.test_mode){
		result.code = 0;
		strcpy(result.text, "OK");
		return (res_to_json(&result));
	}

	if((res = json_to_flc_smooth(json, &smooth)) != J_OK){
		strcpy(result.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, result.text);
		result.code = -2;
		return (res_to_json(&result));
	} 

	dbgmsg_v(MSG_TRACE, "[ APP ] setting smooth_control to: '%s'\n", smooth ? "true" : "false");

	pthread_mutex_lock(&flc_mutex);
	result = flc_set_smooth_ctrl(smooth);
	pthread_mutex_unlock(&flc_mutex);

	return (res_to_json(&result));
}

char* app_set_flc_smooth_value(const char* json)
{
	jerr_t res;
	int smooth_value;
	res_t result;
#if 0
	if(app_data.conf.test_mode){
		result.code = 0;
		strcpy(result.text, "OK");
		return (res_to_json(&result));
	}
#endif

	if((res = json_to_flc_smooth_value(json, &smooth_value)) != J_OK){
		strcpy(result.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, result.text);
		result.code = -2;
		return (res_to_json(&result));
	} 

	pthread_mutex_lock(&flc_mutex);
	result = flc_set_smooth_value(smooth_value);
	pthread_mutex_unlock(&flc_mutex);

	return (res_to_json(&result));
}

char* app_get_coolers_state(void)
{
	char* json = NULL;
	coolers_state_t st;
	st.res.code = 0;
	strcpy(st.res.text, "OK");

	// 2 coolers: input and output
	st.arr_size = 2;
	st.arr = (cooler_status_t*)calloc(st.arr_size, sizeof(cooler_status_t));

	if(!st.arr){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "calloc() failed\n");
		return NULL;
	}

	strcpy(st.arr[0].type, "input");
	strcpy(st.arr[1].type, "output");

	pthread_mutex_lock(&app_coolers_mutex);
	sprintf(st.arr[0].start_hour, "%02u", app_data.cwt.start_in.hour);
	sprintf(st.arr[0].start_min, "%02u", app_data.cwt.start_in.min);
	sprintf(st.arr[0].start_sec, "%02u", app_data.cwt.start_in.sec);
	sprintf(st.arr[0].stop_hour, "%02u", app_data.cwt.stop_in.hour);
	sprintf(st.arr[0].stop_min, "%02u", app_data.cwt.stop_in.min);
	sprintf(st.arr[0].stop_sec, "%02u", app_data.cwt.stop_in.sec);
	sprintf(st.arr[0].wp_min, "%u", app_data.cwt.wp_in.min);

	sprintf(st.arr[1].start_hour, "%02u", app_data.cwt.start_out.hour);
	sprintf(st.arr[1].start_min, "%02u", app_data.cwt.start_out.min);
	sprintf(st.arr[1].start_sec, "%02u", app_data.cwt.start_out.sec);
	sprintf(st.arr[1].stop_hour, "%02u", app_data.cwt.stop_out.hour);
	sprintf(st.arr[1].stop_min, "%02u", app_data.cwt.stop_out.min);
	sprintf(st.arr[1].stop_sec, "%02u", app_data.cwt.stop_out.sec);
	sprintf(st.arr[1].wp_min, "%u", app_data.cwt.wp_out.min);

	st.arr[0].power = app_data.cl_in_power;
	st.arr[0].is_on = app_data.cl_in_on;
	st.arr[1].power = app_data.cl_out_power;
	st.arr[1].is_on = app_data.cl_out_on;
	pthread_mutex_unlock(&app_coolers_mutex);

	return coolers_state_to_json(&st);
}

char* app_save_coolers_power(const char* json)
{
	jerr_t ret;
	coolers_pwr_t cooler_pwr;
	res_t result;
	int res;

	result.code = 0;
	strcpy(result.text, "OK");

	if((ret = json_to_coolers_power(json, &cooler_pwr)) != J_OK){
		strcpy(result.text, jsont_get_err_text(ret));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, result.text);
		EXIT(-2);
	}

	if(cooler_pwr.in >= 0) {
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving input cooler power: %u\n", cooler_pwr.in);
		pthread_mutex_lock(&app_coolers_mutex);
		app_data.cl_in_power = (uint8_t)cooler_pwr.in;
		pthread_mutex_unlock(&app_coolers_mutex);
		if(db.save_cl_power(input, (uint8_t)cooler_pwr.in) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-5);
		}
		// Use new power value if cooler is ON already
		if(app_data.cl_in_on){
			if(set_coolers_power(input, (uint8_t)cooler_pwr.in) != SENS_OK){
				strcpy(result.text, "i2c send error");
				EXIT(-13);
			}
		}
	}
	else{
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving output cooler power: %u\n", cooler_pwr.out);
		pthread_mutex_lock(&app_coolers_mutex);
		app_data.cl_out_power = (uint8_t)cooler_pwr.out;
		pthread_mutex_unlock(&app_coolers_mutex);
		if(db.save_cl_power(output, (uint8_t)cooler_pwr.out) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-5);
		}
		// Use new power value if cooler is ON already
		if(app_data.cl_out_on){
			if(set_coolers_power(output, (uint8_t)cooler_pwr.out) != SENS_OK){
				strcpy(result.text, "i2c send error");
				EXIT(-13);
			}
		}
	}

exit:
	result.code = res;
	return (res_to_json(&result));
}

static int save_new_coolers_time(my_time_t* dest, coolers_raw_t db_raw_name, my_time_t* new_time)
{
	int res = MYDB_OK;

	pthread_mutex_lock(&app_coolers_mutex);
	memcpy(dest, new_time, sizeof(my_time_t));
	pthread_mutex_unlock(&app_coolers_mutex);

	// db set corresponding time
	res = db.set_cl_time(db_raw_name, new_time);

	return res;
}

char* app_set_coolers_time(const char* json, void* flag)
{
	jerr_t ret;
	my_time_t tm;
	res_t result;
	int res = 0;
	strcpy(result.text, "OK");

	if((ret = json_to_my_time(json, &tm)) != J_OK){
		strcpy(result.text, jsont_get_err_text(ret));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, result.text);
		EXIT(-2);
	} 

	if(flag == CL_START_IN){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Input cooler start time: %02u:%02u:%02u\n", tm.hour, tm.min, tm.sec);
		if(save_new_coolers_time(&app_data.cwt.start_in, start_in_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}
	else if(flag == CL_STOP_IN){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Input cooler stop time: %02u:%02u:%02u\n", tm.hour, tm.min, tm.sec);
		if(save_new_coolers_time(&app_data.cwt.stop_in, stop_in_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}
	else if(flag == CL_WP_IN){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Input cooler work period time: %02u min\n", tm.min);
		if(save_new_coolers_time(&app_data.cwt.wp_in, wp_in_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}
	if(flag == CL_START_OUT){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Output cooler start time: %02u:%02u:%02u\n", tm.hour, tm.min, tm.sec);
		if(save_new_coolers_time(&app_data.cwt.start_out, start_out_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}
	else if(flag == CL_STOP_OUT){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Output cooler stop time: %02u:%02u:%02u\n", tm.hour, tm.min, tm.sec);
		if(save_new_coolers_time(&app_data.cwt.stop_out, stop_out_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}
	else if(flag == CL_WP_OUT){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Saving Output cooler work period time: %02u min\n", tm.min);
		if(save_new_coolers_time(&app_data.cwt.wp_out, wp_out_time, &tm) != MYDB_OK){
			strcpy(result.text, "sqlite error");
			EXIT(-3);
		}
	}

	// Force coolers thread to work
	app_cl_wakeup();

exit:
	result.code = res;
	return (res_to_json(&result));
}

char* app_set_lan(const char* json)
{
	jerr_t res;
	lan_sett_t lan;
	res_t lan_res;

	if((res = json_to_lan_settings(json, &lan)) != J_OK){
		strcpy(lan_res.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, lan_res.text);
		lan_res.code = -2;
		return (res_to_json(&lan_res));
	}

	pthread_mutex_lock(&settings_mutex);

	if(lan.dhcp){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Turning DHCP On. Using Dynamic IP\n");
		lan_res.code = set_dynamic_ip(app_data.conf.lan_conn_name);
		if(lan_res.code) strcpy(lan_res.text, "set_dynamic_ip failed");
		else {
			strcpy(lan_res.text, "OK");
			pthread_mutex_lock(&app_data_mutex);
			app_data.nm_restart_needed = true;
			pthread_mutex_unlock(&app_data_mutex);
		}
	}
	else{
		char ip_str[32];
		char gw_str[32];
		memset(ip_str, 0, sizeof(ip_str));
		memset(gw_str, 0, sizeof(gw_str));
		sprintf(ip_str, "%s.%s.%s.%s", lan.ip4, lan.ip3, lan.ip2, lan.ip1);
		sprintf(gw_str, "%s.%s.%s.%s", lan.gw4, lan.gw3, lan.gw2, lan.gw1);

		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Turning DHCP Off. Using Static IP: '%s', GW: '%s'\n", ip_str, gw_str);

		lan_res.code = set_static_ip(app_data.conf.lan_conn_name, ip_str, gw_str);
		if(lan_res.code) strcpy(lan_res.text, "set_static_ip failed");
		else {
			strcpy(lan_res.text, "OK");
			pthread_mutex_lock(&app_data_mutex);
			app_data.nm_restart_needed = true;
			pthread_mutex_unlock(&app_data_mutex);
		}
	}
	
	pthread_mutex_unlock(&settings_mutex);

	return (res_to_json(&lan_res));
}

char* app_set_wifi_ap(bool on_off)
{
	res_t res;
	char fname[256] = {0};
	memset(fname, 0, sizeof(fname));
	strcat(fname, app_data.data_dir);
	strcat(fname, "/my_wifi_ap_state.txt");
	strcpy(res.text, "OK");
	res.code = 0;

	if(app_data.conf.test_mode) return (res_to_json(&res));

	pthread_mutex_lock(&settings_mutex);

	res.code = set_wifi_ap_state(app_data.conf.wifi_ap_script_path, fname, on_off);
	if(res.code) strcpy(res.text, "Internal error");

	pthread_mutex_unlock(&settings_mutex);

	return (res_to_json(&res));
}


char* app_set_rtc(const char* json)
{
	jerr_t res;
	rtc_sett_t rtc;
	res_t rtc_res;

	if((res = json_to_rtc_settings(json, &rtc)) != J_OK){
		strcpy(rtc_res.text, jsont_get_err_text(res));
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, rtc_res.text);
		rtc_res.code = -2;
		return (res_to_json(&rtc_res));
	}

	pthread_mutex_lock(&settings_mutex);

	if(strcmp(rtc.hour, "") && strcmp(rtc.min, "") && strcmp(rtc.sec, "")){
		dbgmsg_v(MSG_VERBOSE, "Setting time %s:%s:%s\n", rtc.hour, rtc.min, rtc.sec);
		rtc_res.code = set_rtc_time(rtc.hour, rtc.min, rtc.sec);
		if (rtc_res.code) strcpy(rtc_res.text, "Set time failed");
		else strcpy(rtc_res.text, "OK");
	}
	else if(strcmp(rtc.year, "") && strcmp(rtc.month, "") && strcmp(rtc.mday, "")){
		dbgmsg_v(MSG_VERBOSE, "Setting date %s.%s.%s\n", rtc.mday, rtc.month, rtc.year);
		rtc_res.code = set_rtc_date(rtc.mday, rtc.month, rtc.year);
		if (rtc_res.code) strcpy(rtc_res.text, "Set date failed");
		else strcpy(rtc_res.text, "OK");
	} 
	else{
		strcpy(rtc_res.text, "Not enough data");
		rtc_res.code = -5;
	}
	
	pthread_mutex_unlock(&settings_mutex);

	return (res_to_json(&rtc_res));
}


char* app_reboot(void)
{
	res_t res;

	res.code = 0;
	strcpy(res.text, "OK");

	pthread_mutex_lock(&app_data_mutex);
	if(app_data.conf.remote_reboot) app_data.reboot_needed = true;
	pthread_mutex_unlock(&app_data_mutex);

	return (res_to_json(&res));
}

char* app_poweroff(void)
{
	res_t res;

	res.code = 0;
	strcpy(res.text, "OK");

	pthread_mutex_lock(&app_data_mutex);
	if(app_data.conf.remote_poweroff) app_data.poweroff_needed = true;
	pthread_mutex_unlock(&app_data_mutex);

	return (res_to_json(&res));
}

char* app_restore_dflt(void)
{
	res_t res;

	res.code = 0;
	strcpy(res.text, "OK");

	// TODO

	return (res_to_json(&res));
}

//############################### [ CLIMATE ROUTINE ] ###############################

char* app_set_th_period(const char* json)
{
	int res = 0;
	res_t resp;
	jerr_t ret = J_OK;
	int new_period_min = 0;
	uint64_t new_period_sec = 0;

	if( (ret = json_to_th_period(json, &new_period_min)) != J_OK){
		strcpy(resp.text, jsont_get_err_text(ret));
		EXIT((int)ret);
	}

	new_period_sec = new_period_min * 60;
	if(new_period_sec < 180){
		strcpy(resp.text, "New period is too small. Min 3 min");
		EXIT(-4);
	}

	if(db.set_th_period(new_period_sec) != MYDB_OK){
		strcpy(resp.text, "sqlite error");
		EXIT(-5);
	}

	strcpy(resp.text, "OK");

	pthread_mutex_lock(&app_data_mutex);
	app_data.conf.th_poll_period = new_period_sec;
	pthread_mutex_unlock(&app_data_mutex);

	app_th_wakeup();

exit:
	resp.code = res;
	return res_to_json(&resp);
}

char* app_get_avail_th_dates(void)
{
	int ret = 0;
	char* json = NULL;
	th_avail_resp_t resp;

	ret = db.get_th_dates();

	if(ret != MYDB_OK){
		resp.res.code = -1;
		strcpy(resp.res.text, "db request failed");
	}

	resp.arr_size = db.th_dates.size();
	resp.arr = (th_adate_t*)calloc(resp.arr_size, sizeof(th_adate_t));
	if(!resp.arr){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "calloc() failed\n");
		resp.res.code = -2;
		strcpy(resp.res.text, "memory allocation failed");
	}
	else{
		for(size_t i = 0; i < resp.arr_size; i++){
			strcpy(resp.arr[i].date, db.th_dates[i].full_date_str);
		}

		strcpy(resp.res.text, "OK");
	}
	
	json = th_avail_to_json(&resp);

	db.th_dates.clear();
	free(resp.arr);

	return json;
}

char* app_get_th_day_data(const char* json)
{
	int ret = 0;
	int res = 0;
	char* resp_json = NULL;
	th_data_resp_t resp;
	th_date_req_t req;

	strcpy(resp.res.text, "OK");

	th_date_t req_date;
	th_time_t start_time;
	th_time_t end_time;

	// Parse selected date
	if ((ret = json_to_th_date(json, &req)) != J_OK){
		strcpy(resp.res.text, jsont_get_err_text((jerr_t)ret));
		EXIT(ret);
	}

	req_date.date = req.day;
	req_date.month = req.month;
	req_date.year = (req.year >= 2020) ? req.year : 0;
	end_time.hour = 23;
	end_time.min = 59;

	ret = db.get_th_val(&req_date, &start_time, &end_time);

	if(ret != MYDB_OK){
		strcpy(resp.res.text, "db request failed");
		EXIT(-1);
	}

	resp.arr_size = db.th_records.size();
	resp.arr = (th_data_t*)calloc(resp.arr_size, sizeof(th_data_t));
	if(!resp.arr){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "calloc() failed\n");
		strcpy(resp.res.text, "memory allocation failed");
		EXIT(-2);
	}

	for(size_t i = 0; i < resp.arr_size; i++){
		strcpy(resp.arr[i].time, db.th_records[i].time_str);
		resp.arr[i].temp = db.th_records[i].temp;
		resp.arr[i].hum = db.th_records[i].hum;
	}

	db.th_records.clear();

exit:
    resp.res.code = res;
	resp_json = th_data_to_json(&resp);
    if(resp.arr) free(resp.arr);
    return resp_json;
}

char* app_get_th_data(void* flag)
{
	int ret = 0;
	int res = 0;
	char* json = NULL;
	th_data_resp_t resp;
	strcpy(resp.res.text, "OK");

	th_date_t curr_date;
	th_time_t start_time;
	th_time_t end_time;

	struct tm ct;
	if(!get_local_time(&ct)){	
		strcpy(resp.res.text, "get_local_time failed");
		EXIT(-1);
	}

	if(flag == TH_LATEST) 
	{
    dbgmsg_v(MSG_TRACE, "Getting 'latest' TH data..\n");

    curr_date.date = ct.tm_mday;
		curr_date.month = ct.tm_mon;
		curr_date.year = ct.tm_year;

		if(ct.tm_min > 30){
			start_time.hour = ct.tm_hour;
			start_time.min = ct.tm_min - 30;
			end_time.hour = ct.tm_hour;
			end_time.min = ct.tm_min;
		}
		else{
			start_time.hour = ct.tm_hour - 1;
			start_time.min = 30 + ct.tm_min;
			end_time.hour = ct.tm_hour;
			end_time.min = ct.tm_min;
		}
        
		ret = db.get_th_val(&curr_date, &start_time, &end_time);
		uint64_t tmp = 0;
		if (db.get_th_period(&tmp) == MYDB_OK){
			resp.poll_period = tmp / 60; // [min]
		}

		if(ret != MYDB_OK){
			strcpy(resp.res.text, "db request failed");
			EXIT(-1);
		}

		resp.arr_size = db.th_records.size();
		resp.arr = (th_data_t*)calloc(resp.arr_size, sizeof(th_data_t));
		if(!resp.arr){
			errmsg_v(MSG_DEBUG | MSG_TO_FILE, "calloc() failed\n");
			strcpy(resp.res.text, "memory allocation failed");
			EXIT(-2);
		}

		for(size_t i = 0; i < resp.arr_size; i++){
			strcpy(resp.arr[i].time, db.th_records[i].time_str);
			resp.arr[i].temp = db.th_records[i].temp;
			resp.arr[i].hum = db.th_records[i].hum;
		}

		db.th_records.clear();
  }
  else if(flag == TH_WEEK){
  	//TODO
    dbgmsg_v(MSG_TRACE, "Getting 'week' TH data..\n");
  }
  else if(flag == TH_MONTH){
  	//TODO
    dbgmsg_v(MSG_TRACE, "Getting 'month' TH data..\n");
  }
  else errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unsupported argument \n");


exit:
  resp.res.code = res;
	json = th_data_to_json(&resp);
  if(resp.arr) free(resp.arr);
  return json;
}