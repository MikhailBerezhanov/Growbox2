#include <pthread.h> 
#include <linux/limits.h>
#include <unistd.h>
#include <inttypes.h>

#include "sensors.h"
#include "app_th.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ APP_TH ] "
#include "dbgmsg.h"

static pthread_t temp_hum_worker_id;
static mydb* db_ptr = NULL;	// pointer at app database object

static pthread_mutex_t th_signal_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool th_signal = false;

static void th_sleep(int period_sec)
{
	while(period_sec--)
	{
		sleep(1);
		pthread_mutex_lock(&th_signal_mutex);
		if(th_signal){
			th_signal = false;
			pthread_mutex_unlock(&th_signal_mutex);
			return;
		}
		pthread_mutex_unlock(&th_signal_mutex);
	}
}

static void* temp_hum_worker(void* arg)
{
	// Enable thread cancelling
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if(!db_ptr) return NULL;
	float temperature = 0;
	float humidity = 0;
	int ret = SENS_OK;

	// period from config file has more priority
	pthread_mutex_lock(&app_data_mutex);
	uint64_t period = app_data.conf.th_poll_period;
	pthread_mutex_unlock(&app_data_mutex);

	if(!period) {
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "No TH period is set (%" PRIu64 ") in config. Reading from db..\n", period);
		
		if(db_ptr->get_th_period(&period) == MYDB_OK){
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
 		db_ptr->check_size(app_data.conf.mydb_max_size);
		// TEST MODE
		if(app_data.conf.test_mode){   
				srand ( time(NULL) );    
		    temperature = 2.25 * (float)(rand() % 20 + 1);
		    humidity = 1.25 * (float)(rand() % 80);
		    dbgmsg_v(MSG_VERBOSE, "[ TEST MODE ] Saving Temp: %f Hum: %f\n", temperature, humidity);
		}
		// WORK MODE
		else ret = sht30_read_data(&temperature, &humidity);

		if(ret == SENS_OK) db_ptr->put_th_val(temperature, humidity);
		else errmsg_v(MSG_DEBUG | MSG_TO_FILE, "sht30_read_data() failed: %d\n", ret);

		// Renew poll period 
		pthread_mutex_lock(&app_data_mutex);
		period = app_data.conf.th_poll_period;
		pthread_mutex_unlock(&app_data_mutex);

		th_sleep(period);
	}

	return NULL;
}

int app_th_init(mydb* db)
{
	if(!db) {
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "No database provided\n");
		return -1;
	}

	db_ptr = db;
	// Gatherer of temeprature and humidity statistics
	if (pthread_create(&temp_hum_worker_id, NULL, temp_hum_worker, NULL)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_create() temp_hum_worker failed\n");
		return -1;
	}

	return 0;
}

void app_th_wakeup(void)
{
	pthread_mutex_lock(&th_signal_mutex);
	th_signal = true;
	pthread_mutex_unlock(&th_signal_mutex);
}

void app_th_deinit(void)
{
	pthread_cancel(temp_hum_worker_id);
	pthread_join(temp_hum_worker_id, NULL);
}