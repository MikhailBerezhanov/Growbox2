#ifndef _APP_DB_H_
#define _APP_DB_H_

#include <stdint.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <string>
#include <vector>

#include "sensors.h"
#include "app_types.h"



// Flags for opening db
#define READONLY				SQLITE_OPEN_READONLY
#define READWRITE				SQLITE_OPEN_READWRITE
#define FULLMUTEX 				SQLITE_OPEN_FULLMUTEX
#define CREATE					SQLITE_OPEN_CREATE

#define MYDB_MAX_SIZE			( 2 * 1048576 )	// 2 MB

// Application tables
//#define TABLE_TEMP_HUM			"TempHum"
//#define TABLE_SYS_CONFIG		"SystemConfig"
//#define TABLE_ACCOUNTS			"Accounts"


class SysConfig_table
{
public:
	void create(sqlite3 *sq);

	void set_th_poll_sec(uint64_t period);
	uint64_t get_th_poll_sec(void);

private:
	sqlite3 *fd = nullptr;
	std::string name = "SystemConfig";
};

class Accounts_table
{

};

class TempHum_table
{
public:
	typedef struct date{
		uint32_t date = 0;
		uint32_t month = 0;
		uint32_t year = 0;
		std::string full_date_str;
		std::string date_str;
	}date_t;

	typedef struct record{
		std::string full_time_str;
		std::string time_str;
		s_time tstamp;
		float temp = 0.0;
		float hum = 0.0;
		uint stamp_min = 0;
	}record_t;

	//TempHum_table(const sqlite3 *sq): fd(sq) {}

	void create(sqlite3 *sq);
	void put_record(float temp, float hum);
	void get_record(date_t *date, s_time *start, s_time *end, std::vector<record_t> *vec);
	void get_record_dates(std::vector<date_t> *vec);

private:
	sqlite3 *fd = nullptr;
	std::string name = "TempHum";
};

class Coolers_table
{
public:
	typedef enum{
		start_in_time = 1,
		stop_in_time,
		wp_in_time,
		start_out_time,
		stop_out_time,
		wp_out_time,
	}period_t;

	typedef struct {
		s_time start_in;
		s_time stop_in;
		s_time wp_in;
		s_time start_out;
		s_time stop_out;
		s_time wp_out;
	}work_time_t;

	void create(sqlite3 *sq);

	int set_time(period_t type, s_time *time);
	int get_time(work_time_t *time);
	int save_power(Sensors::cooler_type type, uint8_t power);
	int get_power(Sensors::cooler_type type, uint8_t *power);

private:
	sqlite3 *fd = nullptr;
	std::string name = "Coolers";
};


namespace app{

class Database
{
public:

#if 1
	void init(const char* path, int flags = READWRITE | FULLMUTEX | CREATE);
	void deinit();
	// max_size - max available size iin bytes
	int check_size(uint64_t max_size);
#endif
	
	SysConfig_table sys_config;
	TempHum_table temp_hum;
	Coolers_table coolers;

private:
	sqlite3 *fd = nullptr;
	std::string path;
};





}//namespace

#endif /*_MYDB_H_*/