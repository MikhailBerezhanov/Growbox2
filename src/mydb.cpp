/* SQLite database connection module.
	Implements data storaging and 
	exchanging with local databases.
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

using namespace std;

#include "utils.h"
#include "mydb.h"

#if DB_MUTUAL
    #include <pthread.h> 
    static pthread_mutex_t mydb_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ MYDB ] "
#include "dbgmsg.h"


typedef int (*sq3_cb_func)(void*, int, char**, char**);

static void lock_db_mutex(void)
{
	#if DB_MUTUAL
	pthread_mutex_lock(&mydb_mutex);
	#endif
}

static void unlock_db_mutex(void)
{
	#if DB_MUTUAL
	pthread_mutex_unlock(&mydb_mutex);
	#endif
}

static bool send_sql(sqlite3 *db, const char* sql, sq3_cb_func cb, void* param) 
{
	char *err = NULL;

	dbgmsg_v(MSG_TRACE, "My SQL is:\n%s\n", sql);

	lock_db_mutex();
	if(sqlite3_exec(db, sql, cb, param, &err) != SQLITE_OK){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "sqlite3_exec() Error: %s\n", err);
    	sqlite3_free(err);
    	unlock_db_mutex();
    	return false;
	}
	unlock_db_mutex();

	return true;
}

typedef struct{
	char name[128];
	bool exists;
}column_info_t;

static int get_table_info_cb(void* param, int argc, char** argv, char** col_name)
{
	column_info_t *ci = (column_info_t*)param;

	dbgmsg_v(MSG_TRACE, "|%s|", argv[1]);
	if( !strcmp(ci->name, argv[1]) ) {
		dbgmsg_v(MSG_TRACE, " <- column found\n");
		ci->exists = true;
	}
	else dbgmsg_v(MSG_TRACE, "\n");

	return 0;
}

static bool add_column_if_not_exists(sqlite3 *db, const char* table_name, const char* col_name, const char* col_type, const char* col_value)
{
	bool res = true;
	char sql[1024];
	
	column_info_t *ci = (column_info_t *)calloc(1, sizeof(column_info_t));
	if(!ci){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to calloc column_info\n");
		return false;
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "PRAGMA table_info(%s);", table_name);
	strncpy(ci->name, col_name, sizeof(ci->name)-1);
	ci->exists = false;

	dbgmsg_v(MSG_TRACE, "Table '%s' contains columns:\n", table_name);
	if(!send_sql(db, sql, get_table_info_cb, ci)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to get '%s' table_info\n", table_name);
		EXIT(false);
	}

	if(!ci->exists){
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "ALTER TABLE %s ADD COLUMN %s %s;", table_name, col_name, col_type);
		if(!send_sql(db, sql, NULL, NULL)){
			errmsg_v(MSG_DEBUG | MSG_TO_FILE, "ADD COLUMN '%s' to '%s' table failed\n", col_name, table_name);
			EXIT(false);
		}
		else{
			dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "Column '%s' added to '%s' table\n", col_name, table_name);
		}

		if(col_value){
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "UPDATE %s SET %s=%s;", table_name, col_name, col_value);
			if(!send_sql(db, sql, NULL, NULL)){
				errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to SET value for '%s' column\n", col_name);
				EXIT(false);
			}
		}
	}
	else{
		dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "Column '%s' already exists in '%s' table\n", col_name, table_name);
	}

exit:
	free(ci);
	return res;
}



mydb::mydb()
{
	this->fd = NULL;
	this->path = "";
}

mydb::~mydb()
{
	if(this->fd) {
		sqlite3_close(this->fd);
		this->fd = NULL;
	}
}

int mydb::init(const char* path, int flags)
{
	if(sqlite3_open_v2(path, &this->fd, flags, NULL)){
		errmsg_v(MSG_DEBUG, "sqlite3_open_v2() Error: %s\n", sqlite3_errmsg(this->fd));
		return MYDB_NOT_OPENED;
	}

	dbgmsg_v(MSG_TRACE, "Sqlite3 threadsafe mode: %s\n", sqlite3_threadsafe() ? "true" : "false");

	// Create application data tables if db is empty
	if(!send_sql(this->fd, "CREATE TABLE IF NOT EXISTS " TABLE_TEMP_HUM " ( \
id INTEGER PRIMARY KEY AUTOINCREMENT, \
date INTEGER, \
month INTEGER, \
year INTEGER, \
hour INTEGER, \
min INTEGER, \
sec INTEGER, \
temp TEXT, \
hum TEXT, \
stamp_min INTEGER);", NULL, NULL))
	{
		errmsg_v(MSG_DEBUG, "Couldn't create '" TABLE_TEMP_HUM "' table\n");
		return MYDB_ERR;
	}

	if(!send_sql(this->fd, "CREATE TABLE IF NOT EXISTS " TABLE_SYS_CONFIG " ( \
id INTEGER PRIMARY KEY AUTOINCREMENT, \
th_poll_period INTEGER, \
cl_start_in_hour INTEGER, \
cl_start_in_min INTEGER, \
cl_start_in_sec INTEGER, \
cl_stop_in_hour INTEGER, \
cl_stop_in_min INTEGER, \
cl_stop_in_sec INTEGER, \
cl_wp_in_min INTEGER, \
cl_start_out_hour INTEGER, \
cl_start_out_min INTEGER, \
cl_start_out_sec INTEGER, \
cl_stop_out_hour INTEGER, \
cl_stop_out_min INTEGER, \
cl_stop_out_sec INTEGER, \
cl_wp_out_min INTEGER, \
cl_in_power INTEGER, \
cl_out_power INTEGER); \
INSERT OR IGNORE INTO " TABLE_SYS_CONFIG " (id, th_poll_period, cl_start_in_hour, cl_start_in_min, \
cl_start_in_sec, cl_stop_in_hour, cl_stop_in_min, cl_stop_in_sec, cl_wp_in_min, cl_start_out_hour, \
cl_start_out_min, cl_start_out_sec, cl_stop_out_hour, cl_stop_out_min, cl_stop_out_sec, cl_wp_out_min, \
cl_in_power, cl_out_power) \
VALUES (1, 600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);", NULL, NULL))
	{
		errmsg_v(MSG_DEBUG, "Couldn't create '" TABLE_SYS_CONFIG "' table\n");
		return MYDB_ERR;
	}

	this->path = path;
	return MYDB_OK;
}

void mydb::deinit()
{
	if(this->fd) {
		sqlite3_close(this->fd);
		this->fd = NULL;
	}

	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ DB closed ~\n");
}

int mydb::check_size(uint64_t max_size)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	uint64_t curr_size = posix_filesize(this->path.c_str());
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Current db size: %" PRIu64 " bytes\n", curr_size);

	if(curr_size >= max_size)
	{
		string sql("DELETE FROM " TABLE_TEMP_HUM " WHERE id < \
(SELECT id FROM " TABLE_TEMP_HUM " ORDER BY id ASC LIMIT 300,1);");

 		if(!send_sql(this->fd, sql.c_str(), NULL, NULL)){
			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Couldn't DELETE FROM '" TABLE_TEMP_HUM "'\n");
			return MYDB_ERR;
		}

		sql = "VACUUM;";

		if(!send_sql(this->fd, sql.c_str(), NULL, NULL)){
			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Couldn't VACUUM '" TABLE_TEMP_HUM "'\n");
			return MYDB_ERR;
		}

		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "[ CLEAN ] '" TABLE_TEMP_HUM " cleaned-up'\n");
		return MYDB_CLEANED_UP;
	}

	return MYDB_OK;
}

int mydb::put_th_val(float temp, float hum)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	char sql[1024];
	memset(sql, 0, sizeof(sql));

	// Time stamp
	struct tm ct;
	if(!get_local_time(&ct)) return MYDB_ERR;

	sprintf(sql, "INSERT INTO " TABLE_TEMP_HUM " (date, month, year, hour, min, sec, temp, hum, stamp_min) \
VALUES (%02u, %02u, %04u, %02u, %02u, %02u, \"%.2f\", \"%.2f\", %u);", 
ct.tm_mday, ct.tm_mon, ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec, temp, hum, ct.tm_hour * 60 + ct.tm_min);

	if(!send_sql(this->fd, sql, NULL, NULL)){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Couldn't INSERT INTO '" TABLE_TEMP_HUM "'\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}

static int get_th_dates_cb(void* param, int argc, char** argv, char** col_name)
{
	vector<th_date_t> *v = (vector<th_date_t>*)param;

	th_date_t tmp;

	sscanf(argv[0], "%" PRIu32 "", &tmp.date);
	sscanf(argv[1], "%" PRIu32 "", &tmp.month);
	sscanf(argv[2], "%" PRIu32 "", &tmp.year);
	sprintf(tmp.date_str, "%02u.%02u", tmp.date, tmp.month);
	sprintf(tmp.full_date_str, "%02u.%02u.%04u", tmp.date, tmp.month, tmp.year);

	v->push_back(tmp);

	return 0;
}

int mydb::get_th_dates()
{
	if(!this->fd) return MYDB_NOT_OPENED;

	this->th_dates.clear();

	if(!send_sql(this->fd, "SELECT DISTINCT date, month, year FROM " TABLE_TEMP_HUM, 
		get_th_dates_cb, &this->th_dates)){
		errmsg_v(MSG_DEBUG, "Unable to select data from '" TABLE_TEMP_HUM "' table\n");
		return MYDB_ERR;
	}

	dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "Available dates from '" TABLE_TEMP_HUM "':\n");
	for(size_t i = 0; i < this->th_dates.size(); i++){
		dbgmsg_v(MSG_VERBOSE, "[%zu] %02" PRIu32 ".%02" PRIu32 ".%04" PRIu32 "\n",
			i, this->th_dates[i].date, this->th_dates[i].month, this->th_dates[i].year);
	}

	return MYDB_OK;
}

static int get_th_val_cb(void* param, int argc, char** argv, char** col_name)
{
	vector<th_table_t> *v = (vector<th_table_t>*)param;

	th_table_t tmp;

	int idx = 4;
	sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.hour);
	sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.min);
	sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.sec);
	sscanf(argv[idx++], "%f", &tmp.temp);
	sscanf(argv[idx++], "%f", &tmp.hum);
	sscanf(argv[idx++], "%u", &tmp.stamp_min);
	sprintf(tmp.full_time_str, "%02" PRIu32 ":%02" PRIu32 ":%02" PRIu32 "", tmp.tstamp.hour, tmp.tstamp.min, tmp.tstamp.sec);
	sprintf(tmp.time_str, "%02" PRIu32 ":%02" PRIu32, tmp.tstamp.hour, tmp.tstamp.min);

	v->push_back(tmp);

	return 0;
}

int mydb::get_th_val(th_date_t* date, th_time_t* start, th_time_t* end)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	char sql[1024];
	memset(sql, 0, sizeof(sql));

	this->th_records.clear();

	if(date->year >= 2020) {
		sprintf(sql, "SELECT * FROM " TABLE_TEMP_HUM " WHERE date=%u AND month=%u AND year=%u \
AND stamp_min>=%u AND stamp_min<=%u;", date->date, date->month, date->year, 
		start->hour * 60 + start->min, end->hour * 60 + end->min);
	}
	else{
		sprintf(sql, "SELECT * FROM " TABLE_TEMP_HUM " WHERE date=%u AND month=%u \
AND stamp_min>=%u AND stamp_min<=%u;", date->date, date->month,  
		start->hour * 60 + start->min, end->hour * 60 + end->min);
	}

	if(!send_sql(this->fd, sql, get_th_val_cb, &this->th_records)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to select data from '" TABLE_TEMP_HUM "' table\n");
		return MYDB_ERR;
	}

	dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "- TH records for '%02u.%02u.%04u' from '%02u:%02u:%02u' till '%02u:%02u:%02u' available: %zu -\n", 
		date->date, date->month, date->year, start->hour, start->min, start->sec, end->hour, end->min, end->sec, this->th_records.size());
	dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "   Time  |\tTemp  |\tHumidity       |\tStamp_min\n");
	dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "---------------------------------------------------------------------\n");
	for(size_t i = 0; i < this->th_records.size(); i++){
		dbgmsg_v(MSG_VERBOSE | MSG_TO_FILE, "%s |\t%.2f | \t%.2f\t |\t  %u\n", this->th_records[i].full_time_str, 
			this->th_records[i].temp, this->th_records[i].hum, this->th_records[i].stamp_min);
	}

	return MYDB_OK;
}

int mydb::set_th_period(uint64_t period)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	char sql[1024];
	memset(sql, 0, sizeof(sql));

	sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET th_poll_period=%" PRIu64 "", period);

	if(!send_sql(this->fd, sql, NULL, NULL)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to update th_poll_period in '" TABLE_TEMP_HUM "' table\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}

static int get_th_period_cb(void* param, int argc, char** argv, char** col_name)
{
	dbgmsg_v(MSG_TRACE, "th_period: %s\n", argv[0]);
	sscanf(argv[0], "%" PRIu64 "", (uint64_t*)param);

	return 0;
}

int mydb::get_th_period(uint64_t* period)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	if(!send_sql(this->fd, "SELECT th_poll_period FROM " TABLE_SYS_CONFIG, get_th_period_cb, period)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to select data from '" TABLE_TEMP_HUM "' table\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}



static int get_cl_time_cb(void* param, int argc, char** argv, char** col_name)
{
	int idx = 0;

	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_in.hour);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_in.min);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_in.sec);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_in.hour);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_in.min);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_in.sec);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->wp_in.min);

	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_out.hour);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_out.min);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->start_out.sec);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_out.hour);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_out.min);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->stop_out.sec);
	sscanf(argv[idx++], "%u", &((coolers_work_time_t*)param)->wp_out.min);

	return 0;
}

int mydb::get_cl_time(coolers_work_time_t* cwt)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	if(!send_sql(this->fd, "SELECT cl_start_in_hour, cl_start_in_min, cl_start_in_sec, cl_stop_in_hour, \
cl_stop_in_min, cl_stop_in_sec, cl_wp_in_min, cl_start_out_hour, cl_start_out_min, cl_start_out_sec, cl_stop_out_hour, \
cl_stop_out_min, cl_stop_out_sec, cl_wp_out_min FROM " TABLE_SYS_CONFIG, get_cl_time_cb, cwt)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to select coolers time from '" TABLE_SYS_CONFIG "' table\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}


int mydb::set_cl_time(coolers_raw_t raw_type, my_time_t* time)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	char sql[1024];
	memset(sql, 0, sizeof(sql));

	switch(raw_type){
		case start_in_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_start_in_hour=%u, \
cl_start_in_min=%u, cl_start_in_sec=%u", time->hour, time->min, time->sec); break;
		case stop_in_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_stop_in_hour=%u, \
cl_stop_in_min=%u, cl_stop_in_sec=%u", time->hour, time->min, time->sec); break;
		case wp_in_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_wp_in_min=%u", time->min); break;
		case start_out_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_start_out_hour=%u, \
cl_start_out_min=%u, cl_start_out_sec=%u", time->hour, time->min, time->sec); break;
		case stop_out_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_stop_out_hour=%u, \
cl_stop_out_min=%u, cl_stop_out_sec=%u", time->hour, time->min, time->sec); break;
		case wp_out_time: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_wp_out_min=%u", time->min); break;
	}

	if(!send_sql(this->fd, sql, NULL, NULL)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to set coolers time in '" TABLE_SYS_CONFIG "' table\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}

static int get_cl_power_cb(void* param, int argc, char** argv, char** col_name)
{
	int idx = 0;

	sscanf(argv[idx++], "%u", (uint*)param);

	return 0;
}

int mydb::get_cl_power(cooler_t type, uint8_t* power)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	uint pwr = 0;
	char sql[1024];
	memset(sql, 0, sizeof(sql));

	switch(type){
		case input: strcpy(sql, "SELECT cl_in_power FROM " TABLE_SYS_CONFIG); break;

		case output: strcpy(sql, "SELECT cl_out_power FROM " TABLE_SYS_CONFIG); break;
	}

	if(!send_sql(this->fd, sql, get_cl_power_cb, &pwr)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to select coolers time from '" TABLE_SYS_CONFIG "' table\n");
		return MYDB_ERR;
	}

	*power = (uint8_t)pwr;

	return MYDB_OK;
}

int mydb::save_cl_power(cooler_t type, uint8_t power)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	char sql[1024];
	memset(sql, 0, sizeof(sql));

	switch(type){
		case input: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_in_power=%u", (uint)power); break;

		case output: sprintf(sql, "UPDATE " TABLE_SYS_CONFIG " SET cl_out_power=%u", (uint)power); break;
	}

	if(!send_sql(this->fd, sql, NULL, NULL)){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to save coolers power in '" TABLE_SYS_CONFIG "' table\n");
		return MYDB_ERR;
	}

	return MYDB_OK;
}