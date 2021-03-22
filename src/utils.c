#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>

#include "utils.h"

#if LOG_FILE_POSIX_MUTEX
    #include <pthread.h> 
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME   "[ UTILS ] "
#include "dbgmsg.h"

static FILE *logfp = NULL;
static char log_fname[640] = {0};
static uint64_t log_max_fsize = MAX_LOG_FILE_SIZE;

int get_cwd(char* buf, size_t buf_len)
{
    int cnt = 0;

    if((cnt = readlink("/proc/self/exe", buf, buf_len)) > 0){
        //buf[cnt] = '\0';
        char *sc = strrchr(buf, '/');
        if(sc) *sc = '\0'; // exclude caller name from path
        dbgmsg_v(MSG_VERBOSE, "working directory: %s\n", buf);
        return 0;
    }
    else perrmsg_v(MSG_DEBUG, "readlink '/proc/self/exe' failed");

    return -1;
}


int get_current_ip (char* fbuf_path, ipv4_t *res)
{
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));
    memset(res, 0, sizeof(ipv4_t));
    
    sprintf(tmp, "ip addr | grep -w inet | grep -v -w lo | cut -d' ' -f 6 > %s", fbuf_path);

    if (system(tmp) ) return -1;

    char* ip_str = read_text_file(fbuf_path, posix_filesize(fbuf_path));

    if(!ip_str) return -2;

    // Considering that we need only one ip from listed with 'ip addr'
    char *end_pos = strchr(ip_str, '/');
    if(end_pos) *end_pos = '\0';    // remove all after '/' sign

    dbgmsg_v(MSG_VERBOSE, "ip_str: %s", ip_str);

    char ipv4[4][4];
    memset(ipv4, 0, sizeof(ipv4));
    // Выделение первой части строки
    char* istr= strtok(ip_str, ".");

    strcpy(res->as_str, ip_str);

    int i = 0;
    if (istr) strcpy(ipv4[i], istr);
    i++;

    // Выделение последующих частей
    while (istr != NULL)
    {
        // Выделение очередной части строки
        istr = strtok(NULL, ".");
        if(istr && (i < 4)){
            strcpy(ipv4[i], istr);
            i++;
        }
    }

    strcpy(res->ip4_as_ch, ipv4[0]);
    strcpy(res->ip3_as_ch, ipv4[1]);
    strcpy(res->ip2_as_ch, ipv4[2]);
    strcpy(res->ip1_as_ch, ipv4[3]);
    res->ip4_as_byte = atoi(res->ip4_as_ch);
    res->ip3_as_byte = atoi(res->ip3_as_ch);
    res->ip2_as_byte = atoi(res->ip2_as_ch);
    res->ip1_as_byte = atoi(res->ip1_as_ch);

    free(ip_str);
    return 0;
}

int get_current_gw(char* fbuf_path, char* iface_name , ipv4_t *res)
{
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));
    memset(res, 0, sizeof(ipv4_t));
    
    sprintf(tmp, "nmcli dev show %s | grep IP4.GATEWAY | cut -d':' -f 2 | tr -d ' ' > %s", iface_name, fbuf_path);

    if (system(tmp) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", tmp);
        return -1;
    }

    char* ip_str = read_text_file(fbuf_path, posix_filesize(fbuf_path));

    if(!ip_str) return -2;

    // Considering that we need only one ip from listed with 'ip addr'
    char *end_pos = strchr(ip_str, '\n');
    if(end_pos) *end_pos = '\0';    // remove all after '/' sign

    dbgmsg_v(MSG_VERBOSE, "ip_str: %s", ip_str);

    char ipv4[4][4];
    memset(ipv4, 0, sizeof(ipv4));
    // Выделение первой части строки
    char* istr= strtok(ip_str, ".");

    strcpy(res->as_str, ip_str);

    int i = 0;
    if (istr) strcpy(ipv4[i], istr);
    i++;

    // Выделение последующих частей
    while (istr != NULL)
    {
        // Выделение очередной части строки
        istr = strtok(NULL, ".");
        if(istr && (i < 4)){
            strcpy(ipv4[i], istr);
            i++;
        }
    }

    strcpy(res->ip4_as_ch, ipv4[0]);
    strcpy(res->ip3_as_ch, ipv4[1]);
    strcpy(res->ip2_as_ch, ipv4[2]);
    strcpy(res->ip1_as_ch, ipv4[3]);
    res->ip4_as_byte = atoi(res->ip4_as_ch);
    res->ip3_as_byte = atoi(res->ip3_as_ch);
    res->ip2_as_byte = atoi(res->ip2_as_ch);
    res->ip1_as_byte = atoi(res->ip1_as_ch);

    free(ip_str);
    return 0;
}

int get_current_dhcp_state(char* fbuf_path, char* conn_name, bool *state)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));
    
    sprintf(cmd, "nmcli c show \"%s\" | grep ipv4.method | cut -d':' -f 2 | tr -d ' ' > %s", conn_name, fbuf_path);

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    char* dhcp_state_str = read_text_file(fbuf_path, posix_filesize(fbuf_path));

    if(!dhcp_state_str) return -2;

    // Considering that we need only one ip from listed with 'ip addr'
    char *end_pos = strchr(dhcp_state_str, '\n');
    if(end_pos) *end_pos = '\0';    // remove all after '/' sign

    dbgmsg_v(MSG_VERBOSE, "dhcp_state_str: %s\n", dhcp_state_str);

    if(!strcmp(dhcp_state_str, "auto")) *state = true;
    else *state = false;

    free(dhcp_state_str);

    return 0;
}

bool restart_network_manager(void)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));
    strcpy(cmd, "nmcli networking off && nmcli networking on");

    dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Restarting Network Manager ~\n");

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return false;
    }

    return true;
}

int set_dynamic_ip(char* conn_name)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));
    
    sprintf(cmd, "nmcli con mod \"%s\" ipv4.addresses \"\" ipv4.gateway \"\" ipv4.dns \"\" ipv4.dns-search \"\" ipv4.method \"auto\" ", conn_name);

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    return 0;
}

int set_static_ip(char* conn_name, char* ip, char* gw)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));
    
    sprintf(cmd, "nmcli con mod \"%s\" ipv4.addresses \"%s\" ipv4.gateway \"%s\" ipv4.dns \"8.8.8.8, 8.8.8.4\" ipv4.method \"manual\" ", 
        conn_name, ip, gw);

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    return 0;
}

int set_wifi_ap_state(char* start_cmd, char* fbuf_path, bool on_off)
{
    int res = 0;
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));

    if(on_off){
        sprintf(cmd, "%s && systemctl status hostapd | grep Active | grep running -c > %s", start_cmd, fbuf_path);
    }
    else strcpy(cmd, "systemctl stop hostapd && ip link set wlan0 down");

    if ( system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    // Check if hostapd service running
    if(on_off){
        char* hostapd_status_str = read_text_file(fbuf_path, posix_filesize(fbuf_path));

        if(!hostapd_status_str) return -2;

        if(!strncmp(hostapd_status_str, "0", 1)) res = -3;   // Service not running

        free(hostapd_status_str);
    }

    return res;
}

int get_wifi_ap_state(char* fbuf_path, bool* state)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));

    sprintf(cmd, "systemctl status hostapd | grep Active | grep running -c > %s", fbuf_path);

    if ( system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    char* hostapd_status_str = read_text_file(fbuf_path, posix_filesize(fbuf_path));

    if(!hostapd_status_str) return -2;

    if(!strncmp(hostapd_status_str, "0", 1)) { *state = false; }  // Service not running
    else { *state = true; }

    free(hostapd_status_str);

    return 0;
}

int set_rtc_time(char* hour, char* min, char* sec)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));

    sprintf(cmd, "date --set \"%s:%s:%s\"", hour, min, sec);

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    return 0;
}

int set_rtc_date(char* mday, char* month, char* year)
{
    char cmd[1024];
    memset(cmd, 0, sizeof(cmd));
    struct tm tm;
    if(!get_local_time(&tm)) return -1;

    sprintf(cmd, "date --set \"%s-%s-%s %02u:%02u:%02u\"", year, month, mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    if (system(cmd) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'%s' failed\n", cmd);
        return -1;
    }

    return 0;
}

int get_cpu_temp(char* temp_file_path, char* cpu_temp)
{
    char* cpu_temp_str = read_text_file(temp_file_path, posix_filesize(temp_file_path));

    if(!cpu_temp_str) return -1;

    dbgmsg_v(MSG_VERBOSE, "CPU temperature: %s", cpu_temp_str);

    long temp = strtol(cpu_temp_str, NULL, 10);

    sprintf(cpu_temp, "%li", temp / 1000);

    free(cpu_temp_str);

    return 0;
}

int system_reboot(void)
{
    int res = 0;

    if( (res = system("reboot") != 0) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'reboot' failed (%d)\n", WEXITSTATUS(res));
        return -1;
    }

    return 0;
}

int system_poweroff(void)
{
    int res = 0;

    if( (res = system("poweroff") != 0) ) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "'poweroff' failed (%d)\n", WEXITSTATUS(res));
        return -1;
    }

    return 0;
}

int make_new_dir(const char* path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        dbgmsg_v(MSG_VERBOSE, "creating '%s' directory\n", path);
        return mkdir(path, 0777);
    }

    dbgmsg_v(MSG_DEBUG, "%s creating '%s': already exists!\n", __FUNCTION__, path);
    return -1; // directory exists
}

int set_timer(long sec_to_expire, timer_exp_cb cb)
{
    struct sigaction sa;
    struct itimerval timer;

    memset (&sa, 0x00, sizeof(sa));
    sa.sa_handler = cb;
    sigaction (SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = sec_to_expire;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) < 0){
        perrmsg_v(MSG_DEBUG,"set_timer() failed ");
        return -1;
    }

    return 0;
}

int start_timeout (struct timespec *time_start) 
{
    return clock_gettime (CLOCK_MONOTONIC, time_start);
}

static unsigned long timediff (const struct timespec t1, const struct timespec t2) 
{
    unsigned long t1_nano = t1.tv_nsec + t1.tv_sec * 1000000000L;
    unsigned long t2_nano = t2.tv_nsec + t2.tv_sec * 1000000000L;

    return t2_nano - t1_nano;
}

bool timeout (const struct timespec time_start, const int ms_timeout) 
{
    struct timespec time_end;
    clock_gettime (CLOCK_MONOTONIC, &time_end);
    return timediff (time_start, time_end) >= (unsigned long)ms_timeout * 1000000L ? true : false;
}

bool get_local_time(struct tm *result)
{
    // currentTime
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm *stm = localtime_r(&spec.tv_sec, result);

    if (!stm) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "localtime() failed: %s\n", strerror(errno));
        return false;
    }
    // Convert to common format
    result->tm_mon++;
    result->tm_year += 1900;

    dbgmsg_v(MSG_TRACE, "Current time: %02u.%02u.%04u %02u:%02u:%02u\n", 
        result->tm_mday, result->tm_mon, result->tm_year, 
        result->tm_hour, result->tm_min, result->tm_sec );

    return true;
}

const char* get_curr_dtime (void) 
{
    static char str[32] = {0};
    // currentTime
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm *stm = localtime(&spec.tv_sec);

    if (!stm) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "localtime() failed: %s\n", strerror(errno));
        return "";
    }

    //19032019_20:30:31.543
    strftime(str, sizeof(str), "%d%m%Y_%H_%M_%S", stm);
    sprintf(str, "%s.%03lu", str, spec.tv_nsec / 1000000L); // nanosec to ms
    //dbgmsg_v(MSG_VERBOSE, "Current time: %s\n", str);

    return str;
}

const char* get_formatted_time (void)
{
    static char str[32] = {0};
    // currentTime
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm *stm = localtime(&spec.tv_sec);

    if (!stm) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "localtime() failed: %s\n", strerror(errno));
        return "";
    }

    //19.03.2019 20:30:31.543
    strftime (str, sizeof(str), "%d.%m.%Y %T", stm);
    sprintf (str, "%s.%03lu", str, spec.tv_nsec / 1000000L); // nanosec to ms
    //dbgmsg ("Current formatted time: %s\n", str);

    return str;
}

uint64_t posix_filesize (const char* fname)
{
    struct stat st;
    if (stat(fname, &st) < 0){
        if(strcmp(fname, log_fname)){
            errmsg_v(MSG_DEBUG | MSG_TO_FILE, "\'%s\' stat failed: %s\n", fname, strerror(errno));
        }
        else{
            printf(DEBUG_MODULE_NAME "\'%s\' stat failed: %s\n", fname, strerror(errno));
        }

        return 0;
    }

    return (uint64_t)st.st_size;
}

size_t filesize(FILE *fp)
{
    long int save_pos, size_of_file;

    save_pos = ftell( fp );
    fseek( fp, 0L, SEEK_END );
    size_of_file = ftell( fp );
    fseek( fp, save_pos, SEEK_SET );
    return( size_of_file );
}

int file_num_in_dir(char* dir_name)
{
    int file_count = 0;
    //int dir_count = 0;
    int total = 0;

    DIR * dirp;
    struct dirent * entry;

    dirp = opendir(dir_name); 
    while ((entry = readdir(dirp)) != NULL) {
        total++;
        if (entry->d_type == DT_REG) {
             file_count++;
        }
    }
    //dir_count = total - file_count;
    closedir(dirp);

    return file_count;
}

// NOTE: free() fnames_t
fnames_t* file_names_in_dir(char* dir_name, int* file_num)
{
    DIR *dir = opendir(dir_name);
    int file_count = 0;
    fnames_t *fnames_ptr = (fnames_t *)calloc(1, sizeof(fnames_t));

    if(dir){
        struct dirent *ent;
        while((ent = readdir(dir)) != NULL){
            if (ent->d_type == DT_REG) {
                file_count++;
                fnames_ptr = (fnames_t *)realloc(fnames_ptr, file_count*sizeof(fnames_t));
                fnames_ptr[file_count-1].idx = file_count;
                strcpy(fnames_ptr[file_count-1].name, ent->d_name);
            }
        }

        *file_num = file_count;
        closedir(dir);
        return fnames_ptr;
    }
    else{
        errmsg_v(MSG_DEBUG, "Error opening directory '%s'\n", dir_name);
        return NULL;
    }
}




void file_write_bytes(FILE *fp, const uint8_t *buf, size_t len)
{
    size_t bytes_written = 0;
    size_t fsize = len;

    while(bytes_written != fsize){
        bytes_written += fwrite(&buf[bytes_written], 1, (fsize - bytes_written), fp);
    }
}

#define CHUNK_SIZE  262144      //1048576
// NOTE: Allocates memory to read file into heap. Needed to be deallocated
uint8_t* read_bin_file(const char* fname, uint64_t fsize)
{
    uint8_t *fdata = (uint8_t*)calloc(fsize+1, sizeof(uint8_t));
    size_t bytes_read = 0;
    uint64_t chunk = 0;
    int fd = open(fname, O_RDONLY);

    if (fd < 0){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to open file '%s'\n", fname);
        return NULL;
    }

    if(!fdata){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to calloc() required size: %lu\n", fsize);
        return NULL;
    }

    memset(fdata, 0, fsize);

    lseek(fd, 0, SEEK_SET);
    dbgmsg_v(MSG_TRACE, "Reading file " _GREEN "'%s'" _RESET " of size: %lu bytes\n", fname, fsize);

    while( chunk < fsize )
    {
        if((bytes_read = read(fd, (fdata + chunk), CHUNK_SIZE)) < 0){
            perrmsg_v(MSG_DEBUG | MSG_TO_FILE, "File read failed\n");
            free(fdata);
            close(fd);
            return NULL;
        }

        chunk += bytes_read;
        dbgmsg_v(MSG_TRACE+1, "bytes_read: %lu\n", bytes_read);
    }
    // Add terminating symb
    fdata[chunk] = '\0';

    close(fd);
    return fdata;
}

// NOTE: Allocates memory to read file into heap. Needed to be deallocated
char* read_text_file(const char* fname, uint64_t fsize)
{
    size_t bytes_read = 0;
    uint64_t chunk = 0;
    int fd = open(fname, O_RDONLY);

    if (fd < 0){
        perrmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to open file '%s'\n", fname);
        return NULL;
    }

    char *fdata = (char*)calloc(fsize+1, sizeof(char));

    if(!fdata){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to calloc() required size: %lu\n", fsize);
        return NULL;
    }

    lseek(fd, 0, SEEK_SET);
    dbgmsg_v(MSG_TRACE, "Reading file " _GREEN "'%s'" _RESET " of size: %lu bytes\n", fname, fsize);

    while( chunk < fsize )
    {
        if((bytes_read = read(fd, (fdata + chunk), CHUNK_SIZE)) < 0){
            perrmsg_v(MSG_DEBUG | MSG_TO_FILE, "File read failed\n");
            free(fdata);
            close(fd);
            return NULL;
        }

        chunk += bytes_read;
        dbgmsg_v(MSG_TRACE+1, "bytes_read: %lu\n", bytes_read);
    }
    // Add terminating symb
    fdata[chunk] = '\0';

    close(fd);
    return fdata;
}



void log_init(const char* fname, uint64_t max_fsize)
{
    strcpy(log_fname, fname);
    if(max_fsize) log_max_fsize = max_fsize;
    else log_max_fsize = MAX_LOG_FILE_SIZE;
}

void log_printf(const char *format, ...)
{
    char str[1024] = {0};
    char time_str[30] = {0};
    char log_str[sizeof(str) + sizeof(time_str)] = {0};
    
    va_list args;
    va_start(args, format);
    vsnprintf(str, sizeof(str), format, args);

    strcat(time_str, "[ ");
    strcat(time_str, get_formatted_time());
    strcat(time_str, " ] ");
    strcat(log_str, time_str);
    strcat(log_str, str);

#if (LOG_FILE_POSIX_MUTEX)
    //pthread_mutex_lock(&log_mutex);
#endif

    if (posix_filesize(log_fname) >= log_max_fsize){
        dbgmsg_v(MSG_DEBUG, "[ CLEAN ] Rotating log..\n");
        logfp = fopen(log_fname, "w+");
        if(logfp) {
            fprintf(logfp, time_str);
            fprintf(logfp, "[ CLEAN ] Log has been rotated\n");
        }
    }
    else logfp = fopen(log_fname, "a+");

    if(logfp){
        fprintf(logfp, log_str);
        fclose(logfp);
    }
    else errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Couldnt open log file '%s'\n", log_fname);   

#if (LOG_FILE_POSIX_MUTEX)
    //pthread_mutex_unlock(&log_mutex);
#endif

    format = va_arg(args, char*);
    va_end(args);
}

uint32_t crc32_wiki(uint32_t crc, const uint8_t *p, size_t len)
{
    const uint8_t *buf = p;
    const uint32_t table [] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
        0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
        0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
        0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
        0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
        0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
        0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
        0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
        0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
        0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
        0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
        0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
        0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
        0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
        0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    while (len--)
        crc = (crc >> 8) ^ table[(crc ^ *buf++) & 0xFF];

    return crc;
}


uint8_t crc8_tab (const uint8_t *block, uint16_t size) 
{
    const uint8_t table[] = {
        0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
        157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
        35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
        190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
        70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
        219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
        101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
        248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
        140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
        17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
        175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
        50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
        202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
        87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
        233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
        116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
    };

    uint8_t crc = 0;

    while (size--) {
        crc = table[crc ^ *block++];
    }

    return crc;
}




uint8_t crc8_sht30(uint8_t *block, uint32_t size)
{

//https://github.com/ControlEverythingCommunity/CE_ARDUINO_LIB/blob/master/SHT30/SHT30.cpp
    const uint8_t POLYNOMIAL = 0x31;
    uint8_t CRC = 0xFF;
    
    for ( int j = size; j; --j )
    {
        CRC ^= *block++;
        for ( int i = 8; i; --i )
        {
            CRC = ( CRC & 0x80 )
            ? (CRC << 1) ^ POLYNOMIAL
            : (CRC << 1);
        }
    }
    return CRC;
}
