#ifndef _MYDB_H_
#define _MYDB_H_

#include <stdint.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <string>
#include <vector>

#include "sensors.h"
#include "jsont.h"

#if APP_MULTITHREADED
	#define DB_MUTUAL	1			// Mutual operations with database
#else 
	#define DB_MUTUAL	0
#endif

using namespace std;

// Flags for opening db
#define READONLY				SQLITE_OPEN_READONLY
#define READWRITE				SQLITE_OPEN_READWRITE
#define FULLMUTEX 				SQLITE_OPEN_FULLMUTEX
#define CREATE					SQLITE_OPEN_CREATE

// Error codes
#define MYDB_OK 				( 0 )
#define MYDB_ERR 				( -1 )
#define MYDB_NOT_OPENED			( -2 )
#define MYDB_CLEANED_UP			( 1 )

#define MYDB_MAX_SIZE			( 2 * 1048576 )	// 2 MB

// Application tables
#define TABLE_TEMP_HUM			"TempHum"
#define TABLE_SYS_CONFIG		"SystemConfig"
#define TABLE_ACCOUNTS			"Accounts"

typedef struct th_date{
	th_date() { memset(this, 0, sizeof(th_date)); };
	uint32_t date;
	uint32_t month;
	uint32_t year;
	char full_date_str[32];
	char date_str[16];
}th_date_t;

typedef struct th_time{
	th_time() { memset(this, 0, sizeof(th_time)); };
	uint32_t min;
	uint32_t hour;
	uint32_t sec;
}th_time_t;

typedef struct th_table{
	th_table() { memset(this, 0, sizeof(th_table)); };
	char full_time_str[32];
	char time_str[16];
	th_time_t tstamp;
	float temp;
	float hum;
	uint stamp_min;
}th_table_t;


typedef enum{
	start_in_time = 1,
	stop_in_time,
	wp_in_time,
	start_out_time,
	stop_out_time,
	wp_out_time,
}coolers_raw_t;

typedef struct {
	my_time_t start_in;
	my_time_t stop_in;
	my_time_t wp_in;
	my_time_t start_out;
	my_time_t stop_out;
	my_time_t wp_out;
}coolers_work_time_t;

class mydb{

public:
	// NOTE: Clear after use
	vector<th_table_t> th_records;	// Temperature and Humidity list
	vector<th_date_t> th_dates;		// Available date-list of TempHum table

	mydb();
	~mydb();
	int init(const char* path, int flags);
	void deinit(void);
	// max_size - max available size iin bytes
	int check_size(uint64_t max_size);
	int put_th_val(float temp, float hum);
	int get_th_val(th_date_t *date, th_time_t *start, th_time_t *end);
	int get_th_dates();
	int set_th_period(uint64_t period);
	int get_th_period(uint64_t* period);

	int set_cl_time(coolers_raw_t raw_type, my_time_t* time);
	int get_cl_time(coolers_work_time_t* time);
	int save_cl_power(cooler_t type, uint8_t power);
	int get_cl_power(cooler_t type, uint8_t* power);
private:
	sqlite3 *fd;
	string path;
};







#endif /*_MYDB_H_*/