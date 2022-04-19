#ifndef __utils_h
#define __utils_h

#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if APP_MULTITHREADED
	#define LOG_FILE_POSIX_MUTEX	1			// Mutual operations with log-file
#else 
	#define LOG_FILE_POSIX_MUTEX	0
#endif

#define MAX_LOG_FILE_SIZE 	2 * 1048576		// 2 [MB]

#define EXIT(x)		do{ res = x; goto exit; }while(0);

#define TO_STRING(x)	( #x )

typedef enum{
	_unknown = 0, 
	_string,
	_uinteger32,
	_uinteger64,
	_integer,
	_character,
	_floating,
	_boolean,
}obj_t;

#ifdef __cplusplus
	#define typeof(x)		_unknown 	// C++ doesn't support _Generic
#else 
	#define typeof(x)		_Generic((x), \
							int: _integer, \
							uint32_t: _uinteger32, \
							uint64_t: _uinteger64, \
							char: _character, \
							char*: _string, \
							float: _floating, \
							bool: _boolean, \
							default: _unknown)

	#define formatof(x)		_Generic((data->x), \
							_int: 	"%d", \
							_char:	"%c", \
							_char*:	"%s", \
							_uint32_t:	"%u", \
							_uint64_t:	"%lu", \
							default:	"0x%X"	)
#endif

typedef struct{
	char name[128];
	uint32_t idx;
}fnames_t;

typedef struct{
	char as_str[32];
	char ip4_as_ch[4];
	char ip3_as_ch[4];
	char ip2_as_ch[4];
	char ip1_as_ch[4];
	uint8_t ip4_as_byte;
	uint8_t ip3_as_byte;
	uint8_t ip2_as_byte;
	uint8_t ip1_as_byte;
}ipv4_t;

typedef void (*timer_exp_cb) (int);

// get current work directory
int get_cwd(char* buf, size_t buf_len);

int get_current_ip (char* fbuf_path, ipv4_t *res);
int get_current_gw(char* fbuf_path, char* iface_name, ipv4_t *res);
int get_current_dhcp_state(char* fbuf_path, char* conn_name, bool *state);

bool restart_network_manager(void);
int set_dynamic_ip(char* conn_name);
int set_static_ip(char* conn_name, char* ip, char* gw);

int set_wifi_ap_state(char* start_cmd, char* fbuf_path, bool on_off);
int get_wifi_ap_state(char* fbuf_path, bool* state);

int set_rtc_time(char* hour, char* min, char* sec);
int set_rtc_date(char* mday, char* month, char* year);

int get_cpu_temp(char* temp_file_path, char* cpu_temp);

int system_reboot(void);
int system_poweroff(void);

int make_new_dir(const char* path);

int set_timer(long sec_to_expire, timer_exp_cb cb);

int start_timeout (struct timespec *time_start);
bool timeout (const struct timespec time_start, const int ms_timeout);

bool get_local_time(struct tm *result);
const char* get_curr_dtime (void); 
const char* get_formatted_time (void);

size_t filesize(FILE *fp);
uint64_t posix_filesize (const char* fname);

int encrypt_file (const char *src, const char *dest, uint32_t psu_number);

int file_num_in_dir(char* dir_name);
fnames_t *file_names_in_dir(char *dir_name, int *file_num);

void file_write_bytes(FILE *fp, const uint8_t *buf, size_t len);
// NOTE: Allocates memory to read file into heap. Needed to be deallocated
char* read_text_file(const char* fname, uint64_t fsize);
uint8_t* read_bin_file(const char* fname, uint64_t fsize);

void log_init(const char* fname, uint64_t max_fsize);
void log_printf(const char *format, ...);

uint32_t crc32_wiki(uint32_t crc, const uint8_t *p, size_t len);
uint8_t crc8_tab(const uint8_t *block, uint16_t size);
uint8_t crc8_sht30(uint8_t *block, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif