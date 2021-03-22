#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h> 
#include <unistd.h>

#include "utils.h"
#include "sensors.h"
#include "app.h"
#include "app_cl.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ CL ] "
#include "dbgmsg.h"

// Thread to work with coolers
static pthread_t coolers_worker_id;

static pthread_mutex_t cl_signal_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool cl_signal = false;

typedef struct 
{
	cooler_t type;
	char type_str[16];
	uint8_t power;
	uint start_time_min;
	uint stop_time_min;
	uint wp_time_min;
	int intervals_num;
	bool is_on;
}cl_time_t;

static void check_cooler_schedule(bool test_mode, uint curr_time_min, cl_time_t* cl_time)
{
	if(cl_time->stop_time_min > cl_time->start_time_min){
		cl_time->intervals_num = (cl_time->stop_time_min - cl_time->start_time_min) / 60;
		dbgmsg_v(MSG_VERBOSE, "[ %s ] Cooler intervals_num: %d From: %02u:%02u To: %02u:%02u \
Every %u min, With power: %u. State: %d\n", cl_time->type_str, cl_time->intervals_num, cl_time->start_time_min / 60, cl_time->start_time_min % 60, 
		cl_time->stop_time_min / 60, cl_time->stop_time_min % 60, cl_time->wp_time_min, cl_time->power, cl_time->is_on);		
	}
	else{
		dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] Cooler start_time <= stop_time. Not checking schedule\n", cl_time->type_str);
		return;
	}

	if(curr_time_min < cl_time->start_time_min){
		dbgmsg_v(MSG_VERBOSE, "[ %s ] Cooler curr_time < start_time. Not checking schedule\n", cl_time->type_str);
		return;
	} 

	// For ON cooler's state
	if(cl_time->is_on)
	{
		bool power_off = false;
		// check stop time
		if(curr_time_min >= cl_time->stop_time_min){	
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] ON: Cooler curr_time >= stop_time\n", cl_time->type_str);		
			power_off = true;
		}
		// check work periods
		else{
			int i = 0;
			bool is_in_interval = false;

			// determine in which interval current time is
			for(i = 0; i <= cl_time->intervals_num; i++){
				if( (curr_time_min >= cl_time->start_time_min + 60*i) &&
						(curr_time_min < cl_time->start_time_min + 60*i + 60) ){
					dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] ON: Cooler curr_time is in '%d' interval \n", cl_time->type_str, i);
					is_in_interval = true;
					break;
				} 
			}

			if( is_in_interval && (curr_time_min >= (cl_time->start_time_min + 60*i + cl_time->wp_time_min)) ){
				dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] ON: Cooler curr_time > wp_time \n", cl_time->type_str);
				power_off = true;
			}
		}

		// Turn cooler OFF
		if(power_off){
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] ON: Cooler turning off..\n", cl_time->type_str);

			if(test_mode){
				cl_time->is_on = false;
				dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ TEST MODE ] [ %s ] ON: Cooler turned off\n", cl_time->type_str);
			}
			else{
				if(set_coolers_power(cl_time->type, 0) == SENS_OK){
					cl_time->is_on = false;
					dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] ON: Cooler turned off\n", cl_time->type_str);
				} 
			}
		}
	}
	// For OFF cooler's state
	else
	{
		bool power_on = false;
		int i = 0;
		bool is_in_interval = false;

		if(curr_time_min >= cl_time->stop_time_min){
			dbgmsg_v(MSG_TRACE, "[ %s ] OFF: Cooler curr_time >= stop_time\n", cl_time->type_str);
			return;
		} 

		// determine in which interval current time is
		for(i = 0; i <= cl_time->intervals_num; i++){
			if( (curr_time_min >= cl_time->start_time_min + 60*i) &&
					(curr_time_min < cl_time->start_time_min + 60*i + 60) ){
				dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] OFF: Cooler curr_time is in '%d' interval \n", cl_time->type_str, i);
				is_in_interval = true;
				break;
			} 
		}

		if( is_in_interval && (curr_time_min < (cl_time->start_time_min + 60*i + cl_time->wp_time_min)) ){
				power_on = true;	
				dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] OFF: Cooler curr_time is in work_period\n", cl_time->type_str);
		}
		else{
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] OFF: Cooler curr_time is NOT in work_period\n", cl_time->type_str);
			return;
		}

		// Turn cooler ON
		if(power_on){
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] OFF: Cooler turning on..\n", cl_time->type_str);

			if(test_mode){
				cl_time->is_on = true;
				dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ TEST MODE ] [ %s ] OFF: Cooler turned on\n", cl_time->type_str);
			}
			else{
				if(set_coolers_power(cl_time->type, cl_time->power) == SENS_OK){
					cl_time->is_on = true;
					dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "[ %s ] OFF: Cooler turned on\n", cl_time->type_str);
				} 
			}
		}
	}
}

static void cl_sleep(int period_sec)
{
	while(period_sec--)
	{
		sleep(1);
		pthread_mutex_lock(&cl_signal_mutex);
		if(cl_signal){
			cl_signal = false;
			pthread_mutex_unlock(&cl_signal_mutex);
			return;
		}
		pthread_mutex_unlock(&cl_signal_mutex);
	}
}

static void* coolers_worker(void* arg)
{
	// Enable thread cancelling
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	struct tm curr_time;

	pthread_mutex_lock(&app_data_mutex);
	bool test_mode = app_data.conf.test_mode;
	int period = app_data.conf.cl_poll_period;
	pthread_mutex_unlock(&app_data_mutex);
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Settings Coolers check work time period to: %d\n", period);

	if(!period){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "No period provided. Cancelling thread\n");
		return NULL;
	}

	uint curr_time_min = 0;
	cl_time_t cl_input;	// in order not to spam i2c every poll period, check state flags
	cl_time_t cl_output;
	memset(&cl_input, 0, sizeof(cl_input));
	memset(&cl_output, 0, sizeof(cl_output));
	cl_input.type = input;
	strcpy(cl_input.type_str, "Input");
	cl_output.type = output;
	strcpy(cl_output.type_str, "Output");

	bool check_schedule = false;

	for(;;)
	{
		// Check if coolers are supposed to work
		check_schedule = false;
		pthread_mutex_lock(&app_coolers_mutex);
		// Input cooler	
		if(app_data.cwt.wp_in.min){
			cl_input.start_time_min = app_data.cwt.start_in.hour*60 + app_data.cwt.start_in.min;
			cl_input.stop_time_min = app_data.cwt.stop_in.hour*60 + app_data.cwt.stop_in.min;
			cl_input.wp_time_min = app_data.cwt.wp_in.min;
			cl_input.power = app_data.cl_in_power;
			check_schedule = true;
		}
		// Output cooler
		if(app_data.cwt.wp_out.min){
			cl_output.start_time_min = app_data.cwt.start_out.hour*60 + app_data.cwt.start_out.min;
			cl_output.stop_time_min = app_data.cwt.stop_out.hour*60 + app_data.cwt.stop_out.min;
			cl_output.wp_time_min = app_data.cwt.wp_out.min;
			cl_output.power = app_data.cl_out_power;
			check_schedule = true;
		}
		pthread_mutex_unlock(&app_coolers_mutex);

		if(!check_schedule){	
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "No coolers work periods found. Not checking schedule\n");
			goto sleep;
		} 

		if(!get_local_time(&curr_time)) goto sleep;
		curr_time_min = curr_time.tm_hour*60 + curr_time.tm_min;
		dbgmsg_v(MSG_VERBOSE, "Current time: %02u:%02u\n", curr_time_min / 60, curr_time_min % 60);
		// Input cooler work time checking
		check_cooler_schedule(test_mode, curr_time_min, &cl_input);

		// Output cooler work time checking
		check_cooler_schedule(test_mode, curr_time_min, &cl_output);

		// Update app data coolers state
		pthread_mutex_lock(&app_coolers_mutex);
		app_data.cl_in_on = cl_input.is_on;
		app_data.cl_out_on = cl_output.is_on;
		pthread_mutex_unlock(&app_coolers_mutex);

sleep:
		cl_sleep(period);	// check coolers' state every 'period' sec
	}

	return NULL;
}



int app_cl_init(void)
{
	if (pthread_create(&coolers_worker_id, NULL, coolers_worker, NULL)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_create() coolers_worker failed\n");
		return -1;
	}

	return 0;
}

void app_cl_wakeup(void)
{
	pthread_mutex_lock(&cl_signal_mutex);
	cl_signal = true;
	pthread_mutex_unlock(&cl_signal_mutex);
}

void app_cl_deinit(void)
{
	pthread_cancel(coolers_worker_id);
	pthread_join(coolers_worker_id, NULL);
}