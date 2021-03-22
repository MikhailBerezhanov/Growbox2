/* SQLite database connection module.
	Implements data storaging and 
	exchanging with local databases.
*/
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cinttypes>
#include <stdexcept>

#include "utils.h"
#include "app_db.h"


#define LOG_MODULE_NAME		"[ DB ] "
#include "loger.h"

#define to_s(x) 	std::to_string(x)

typedef int (*sq3_cb_func)(void*, int, char**, char**);

static void send_sql(sqlite3 *db, const char *sql, sq3_cb_func cb, void* param) 
{
	char *err = nullptr;

	if(!db)	throw std::runtime_error(excp_msg("Np db handle provided"));
	if(!sql) throw std::runtime_error(excp_msg("Np sql provided"));

	log_msg(MSG_OVERTRACE, "My SQL is:\n%s\n", sql);

	if(sqlite3_exec(db, sql, cb, param, &err) != SQLITE_OK){
    	sqlite3_free(err);
    	throw std::runtime_error(excp_msg("sqlite3_exec() failed: " + (std::string)err));
	}
}

#if 0
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

	column_info_t *ci = (column_info_t *)calloc(1, sizeof(column_info_t));
	if(!ci){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to calloc column_info\n");
		return false;
	}

	char *sql = sqlite3_mprintf("PRAGMA table_info(%s);", table_name);

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
 	sqlite3_free(sql);
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
#endif

void SysConfig_table::create(sqlite3 *sq)
{
	fd = sq;

	std::string sql = "CREATE TABLE IF NOT EXISTS " + name + " ( \
id INTEGER PRIMARY KEY, \
th_poll_period INTEGER); \
INSERT OR IGNORE INTO " + name + " (id, th_poll_period) \
VALUES (1, 600);" ;

	send_sql(this->fd, sql.c_str(), nullptr, nullptr);
}

void SysConfig_table::set_th_poll_sec(uint64_t period)
{
	std::string sql = "UPDATE " + name + " SET th_poll_period=" + std::to_string(period);
	send_sql(fd, sql.c_str(), nullptr, nullptr);
}

uint64_t SysConfig_table::get_th_poll_sec(void)
{
	uint64_t period = 0;

	auto callback = [](void* param, int argc, char** argv, char** col_name) int { 
		log_msg(MSG_TRACE, "th_poll_period: %s\n", argv[0]);
		sscanf(argv[0], "%" PRIu64 "", (uint64_t*)param);
		return 0; 
	};

	std::string sql = "SELECT th_poll_period FROM " + name;
	send_sql(fd, sql.c_str(), callback, &period);

	return period;
}

//
void TempHum_table::create(sqlite3 *sq)
{
	fd = sq;
	// Create application data tables if db is empty
	std::string sql = "CREATE TABLE IF NOT EXISTS " + name + " ( \
id INTEGER PRIMARY KEY AUTOINCREMENT, \
date INTEGER, \
month INTEGER, \
year INTEGER, \
hour INTEGER, \
min INTEGER, \
sec INTEGER, \
temp TEXT, \
hum TEXT, \
stamp_min INTEGER);" ;

	send_sql(fd, sql.c_str(), nullptr, nullptr);
}

//
void TempHum_table::put_record(float temp, float hum)
{
	struct tm ct;
	utils::get_local_time(&ct);

	char* formatted = sqlite3_mprintf("INSERT INTO %s (date, month, year, hour, min, sec, temp, hum, stamp_min) \
VALUES (%02u, %02u, %04u, %02u, %02u, %02u, \"%.2f\", \"%.2f\", %u);", name, 
ct.tm_mday, ct.tm_mon, ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec, temp, hum, ct.tm_hour * 60 + ct.tm_min);

	std::unique_ptr<char, decltype(sqlite3_free)*> sql(formatted, sqlite3_free);

#if 0
	std::string sql = "INSERT INTO " + name + " (date, month, year, hour, min, sec, temp, hum, stamp_min) \
VALUES ( " + to_s(ct.tm_mday) + ", " + to_s(ct.tm_mon) + ", " + to_s(ct.tm_year) + ", " + to_s(ct.tm_hour) + ", "\
+ to_s(ct.tm_min) + ", " + to_s(ct.tm_sec) + ", " + to_s(temp) + ", " + to_s(hum) + ", "\
+ to_s(ct.tm_hour * 60 + ct.tm_min) + ");" ; 
#endif

	send_sql(fd, sql.get(), nullptr, nullptr);
}

//
void TempHum_table::get_record(date_t *d, s_time *start, s_time *end, std::vector<record_t> *vec)
{
	auto callback = [](void *param, int argc, char **argv, char **col_name) int {
		std::vector<record_t> *v = static_cast<std::vector<record_t>*>(param);
		record_t tmp;
		int idx = 4;
		sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.hour);
		sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.min);
		sscanf(argv[idx++], "%" PRIu32 "", &tmp.tstamp.sec);
		sscanf(argv[idx++], "%f", &tmp.temp);
		sscanf(argv[idx++], "%f", &tmp.hum);
		sscanf(argv[idx++], "%u", &tmp.stamp_min);
		tmp.full_time_str = to_s(tmp.tstamp.hour) + ":" + to_s(tmp.tstamp.min) + ":" + to_s(tmp.tstamp.sec);
		tmp.time_str = to_s(tmp.tstamp.hour) + ":" + to_s(tmp.tstamp.min);
		v->push_back(tmp);
		return 0;
	};

	std::string sql;

	if(d->year >= 2020) {
		sql = "SELECT * FROM " + name + " WHERE date=" + to_s(d->date) + " AND month=" + to_s(d->month) + \
" AND year=" + to_s(d->year) + " AND stamp_min>=" + to_s(start->hour * 60 + start->min) + \
" AND stamp_min<=" + to_s(end->hour * 60 + end->min) + ";" ;
	}
	else{
		sql = "SELECT * FROM " + name + " WHERE date=" + to_s(d->date) + " AND month=" + to_s(d->month) + \
" AND stamp_min>=" + to_s(start->hour * 60 + start->min) + " AND stamp_min<=" + to_s(end->hour * 60 + end->min) + ";" ;
	}

	send_sql(fd, sql.c_str(), callback, vec);

	log_msg(MSG_VERBOSE | MSG_TO_FILE, "- TH records for '%02u.%02u.%04u' from '%02u:%02u:%02u' till '%02u:%02u:%02u' available: %zu -\n", 
		d->date, d->month, d->year, start->hour, start->min, start->sec, end->hour, end->min, end->sec, vec->size());
	log_msg(MSG_VERBOSE | MSG_TO_FILE, "   Time  |\tTemp  |\tHumidity       |\tStamp_min\n");
	log_msg(MSG_VERBOSE | MSG_TO_FILE, "---------------------------------------------------------------------\n");
	for(size_t i = 0; i < vec->size(); ++i){
		log_msg(MSG_VERBOSE | MSG_TO_FILE, "%s |\t%.2f | \t%.2f\t |\t  %u\n", (*vec)[i].full_time_str, 
			(*vec)[i].temp, (*vec)[i].hum, (*vec)[i].stamp_min);
	}
}

void TempHum_table::get_record_dates(std::vector<date_t> *vec)
{
	auto callback = [](void *param, int argc, char **argv, char **col_name) int { 
		std::vector<date_t> *v = static_cast<std::vector<date_t>*>(param);
		date_t tmp;
		sscanf(argv[0], "%" PRIu32 "", &tmp.date);
		sscanf(argv[1], "%" PRIu32 "", &tmp.month);
		sscanf(argv[2], "%" PRIu32 "", &tmp.year);
		tmp.date_str = to_s(tmp.date) + "." + to_s(tmp.month);
		tmp.date_str = to_s(tmp.date) + "." + to_s(tmp.month) + "." + to_s(tmp.year);
		v->push_back(tmp);
		return 0; 
	};

	std::string sql = "SELECT DISTINCT date, month, year FROM " + name;

	send_sql(fd, sql.c_str(), callback, vec);

	log_msg(MSG_TRACE | MSG_TO_FILE, "Available dates from '%s':\n", name);
	for(size_t i = 0; i < vec->size(); ++i){
		log_msg(MSG_TRACE, "[%zu] %02" PRIu32 ".%02" PRIu32 ".%04" PRIu32 "\n",
			i, (*vec)[i].date, (*vec)[i].month, (*vec)[i].year);
	}
}


//
void Coolers_table::create(sqlite3 *sq) 
{
	fd = sq;

	std::string sql = "CREATE TABLE IF NOT EXISTS " + name + " ( \
id INTEGER PRIMARY KEY AUTOINCREMENT, \
start_in_hour INTEGER, \
start_in_min INTEGER, \
start_in_sec INTEGER, \
stop_in_hour INTEGER, \
stop_in_min INTEGER, \
stop_in_sec INTEGER, \
wp_in_min INTEGER, \
start_out_hour INTEGER, \
start_out_min INTEGER, \
start_out_sec INTEGER, \
stop_out_hour INTEGER, \
stop_out_min INTEGER, \
stop_out_sec INTEGER, \
wp_out_min INTEGER, \
in_power INTEGER, \
out_power INTEGER); \
INSERT OR IGNORE INTO " + name + " (id, start_in_hour, start_in_min, start_in_sec, \
stop_in_hour, stop_in_min, stop_in_sec, wp_in_min, start_out_hour, start_out_min, start_out_sec, \
stop_out_hour, stop_out_min, stop_out_sec, wp_out_min, in_power, out_power) \
VALUES (1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);" ;

	send_sql(fd, sql.c_str(), nullptr, nullptr);
}

namespace app{

void Database::init(const char* path, int flags)
{
	if(sqlite3_open_v2(path, &fd, flags, nullptr)){
		throw std::runtime_error(excp_msg("sqlite3_open_v2 failed: " + (std::string)sqlite3_errmsg(fd)));
	}

	log_msg(MSG_OVERTRACE, "Sqlite3 threadsafe mode: %s\n", sqlite3_threadsafe() ? "true" : "false");

	sys_config.create(fd);
	temp_hum.create(fd);
	coolers.create(fd);

	this->path = path;
}

void Database::deinit()
{
	if(this->fd) {
		sqlite3_close(this->fd);
		this->fd = nullptr;
	}

	log_msg(MSG_DEBUG | MSG_TO_FILE, "~ DB closed ~\n");
}

#if 0
int mydb::check_size(uint64_t max_size)
{
	if(!this->fd) return MYDB_NOT_OPENED;

	uint64_t curr_size = posix_filesize(this->path.c_str());
	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "Current db size: %" PRIu64 " bytes\n", curr_size);

	if(curr_size >= max_size)
	{
		std::string sql("DELETE FROM " TABLE_TEMP_HUM " WHERE id < \
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
#endif

}//namespace

#ifdef UNIT_TEST
int main(int argc, char* argv[])
{
    Log::init(MSG_TRACE, "Log.log", 100000);

    try {
    	app::Database db;
    	db.init("./tests/my2.db");

    	db.sys_config.set_th_poll_sec(430);
    	db.sys_config.get_th_poll_sec();


    	db.temp_hum.put_record(12.1, 25.0);
    	db.temp_hum.put_record(2.12345678, 99.9731);
    	db.temp_hum.put_record(-0.1, 0);
    	std::vector<TempHum_table::date_t> dates_vec;
    	db.temp_hum.get_record_dates(&dates_vec);

    	s_time start;
    	s_time end(24);
    	std::vector<TempHum_table::record_t> records_vec;
    	db.temp_hum.get_record(&dates_vec[0], &start, &end, &records_vec);
    }
    catch(const std::runtime_error& e){
        log_err( "%s\n", e.what());
        return 1;
    }

    return 0;
}
#endif