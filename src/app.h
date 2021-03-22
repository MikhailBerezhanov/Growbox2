#ifndef _APP_H_ 
#define _APP_H_

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "mydb.h"
#include "flc.h"

#define APP_VERSION			" 1.02"		// MAJMAJ.MINMIN

#define DFLT_CWD			"/opt/growbox"
#define DFLT_LOGIN_TMOUT	60
#define DFL_LOG_MAX_SIZE	256			// MB
#define DFL_DB_MAX_SIZE		256			// MB

#define TH_LATEST   	( (void*)0x01 )
#define TH_DAY      	( (void*)0x02 )
#define TH_WEEK     	( (void*)0x03 )
#define TH_MONTH    	( (void*)0x04 )

#define CL_START_IN   	( (void*)0x01 )
#define CL_STOP_IN      ( (void*)0x02 )
#define CL_WP_IN		( (void*)0x03 )
#define CL_START_OUT   	( (void*)0x04 )
#define CL_STOP_OUT     ( (void*)0x05 )
#define CL_WP_OUT		( (void*)0x06 )

typedef struct{
	bool test_mode;
	int log_level;
	uint64_t log_max_size;		// in bytes
	uint64_t mydb_max_size;		// in bytes
	char lan_iface_name[16];
	char lan_conn_name[128];
	char startup_page[256];
	char login_err_page[256];
	char mainmenu_page[256];
	char _404_page[256];
	char template_page[256];
	char settings_page[256];
	char flc_page[256];
	char water_page[256];
	char temperature_page[256];
	char cooler_page[256];
	bool remote_reboot;
	bool remote_poweroff;
	// [ COOLERS ]
	int cl_poll_period;
	// [ WEB ]
	char username[64];
	char password[64];
	int login_timeout;
	char wifi_ap_script_path[256];
	// [ FLC ]
	char flc_tty[64];
	char i2c_path[64];
	char cpu_temp_path[256];
	// [ TH ]
	uint64_t th_poll_period;
}app_config_t;


typedef struct{
	char cwd[128];
	char log_path[256];
	char config_path[256];
	char cgi_sock_path[256];
	char data_dir[256];
	char db_dir[256];
	bool nm_restart_needed;
	bool reboot_needed;
	bool poweroff_needed;

	// From config File
	app_config_t conf;

	// [ COOLERS ]
	coolers_work_time_t cwt;
	uint8_t cl_in_power;
	uint8_t cl_out_power;
	bool cl_in_on;
	bool cl_out_on;
}app_data_t;

int app_init(void);
void app_deinit(void);

int app_read_config(void);

// Flags for app_get_sensors_data()
#define	GET_TEMP_HUM	0x01
#define GET_COOLERS		0x02
#define GET_LIGHT		0x04
#define GET_WATER		0x08
char* app_get_sensors_data(int flags);
char* app_get_curr_settings(void);

char* app_get_flc_status(void);
char* app_set_flc_ch_pwr(const char* json);
char* app_set_flc_time(flc_time_t type, const char* json);
char* app_set_flc_smooth(const char* json);
char* app_set_flc_smooth_value(const char* json);

char* app_get_coolers_state(void);
char* app_save_coolers_power(const char* json);
char* app_set_coolers_time(const char* json, void* flag);

char* app_set_lan(const char* json);
char* app_set_wifi_ap(bool on_off);
char* app_set_rtc(const char* json);
char* app_reboot(void);
char* app_poweroff(void);
char* app_restore_dflt(void);

char* app_set_th_period(const char* json);
char* app_get_avail_th_dates(void);
char* app_get_th_data(void* flag);
char* app_get_th_day_data(const char* json);

extern app_data_t app_data;
extern pthread_mutex_t app_data_mutex;
extern pthread_mutex_t app_coolers_mutex;

#endif