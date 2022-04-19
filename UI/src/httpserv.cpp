#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>

#include "utils.h"
#include "httpserv.h"
#include "app.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME   "[ HTTPS ] "
#include "dbgmsg.h"

bool logged_in = false;


// Returns pointer at c-string HTTP body (data)
// NOTE: Don't forget to free() 
static char* http_get_body(FCGX_Request *request) 
{
    size_t data_len;

    int i = 0;
    int ch;
    char *content_len = FCGX_GetParam("CONTENT_LENGTH", request->envp);
    char *content_type = FCGX_GetParam("CONTENT_TYPE", request->envp);

    if (content_type && content_len)
    {
        data_len = strtol(content_len, NULL, 10);
        if (data_len > MAX_POST_LEN) {
            errmsg_v(MSG_DEBUG, "data_len overflow\n");
            return NULL;
        }
        char *data = (char*)calloc(1, data_len+1);
        if (data == NULL)
        {
            errmsg_v(MSG_DEBUG, "calloc() failed\n");
            return NULL;
        }
        ch = FCGX_GetChar(request->in);
        while(ch != -1)
        {
            i++;
            data[i-1] = (char)ch;
            ch = FCGX_GetChar(request->in);
            if (ch == -1)
            {
                data = (char*)realloc(data, (i + 1)*sizeof(char));
                data[i] = '\0';
            }
        }           
        dbgmsg_v(MSG_TRACE, "HTTP data:\n%s\n", data);  

        return data; 
    }

    dbgmsg_v(MSG_DEBUG, "CONTENT_LENGTH or CONTENT_TYPE is empty\n");
    return NULL;
}

static void send_html(FCGX_Request *request, const char* html_name)
{
    char fname[512] = {0};
    memset(fname, 0, sizeof(fname));
    strcpy(fname, html_name);

    dbgmsg_v(MSG_VERBOSE, "sending '%s' ..\n", fname);

    size_t fsize = posix_filesize(fname);

    if(fsize){
        char* content = read_text_file(fname, fsize);
        if (!content) return;

        FCGX_PutS("Content-type: text/html\r\n\r\n", request->out);
        FCGX_PutS(content, request->out);
        FCGX_PutS("\r\n", request->out);

        free(content);
    }  
}

static void send_json(FCGX_Request *request, char* json)
{
    FCGX_PutS("Content-type: application/json\r\n\r\n", request->out);
    FCGX_PutS(json, request->out);
    FCGX_PutS("\r\n", request->out);
}

static void serve_html(FCGX_Request *request, void* arg)
{
    char* page_name = NULL;

    if(arg) {
        page_name = (char*)arg;
        send_html(request, page_name);
    }
    else dbgmsg_v(MSG_DEBUG, "WARNING: NO PAGE NAME\n");
}

static bool user_sign_in(char *buf, const char *log, const char *pass)
{
    char *log_ptr = NULL;
    char *pass_ptr = NULL;
    
    if (NULL != (log_ptr = strstr(buf, "username="))){
        if (NULL != (pass_ptr = strstr(log_ptr, "&pass="))){   
            *pass_ptr = '\0';       // now we have 'login' as c-string
            
            //dbgmsg("Login: %s, Password: %s\r\n", &log_ptr[9], &pass_ptr[6]);
            
            if (!strcmp(&log_ptr[9], log) && !strcmp(&pass_ptr[6], pass)) return true;
            else dbgmsg_v(MSG_VERBOSE, "Sign-in failed (%s:%s)\n", &log_ptr[9], &pass_ptr[6]);               
        }   
    }
    return false;
}

static void login_timer_handler(int signum)
{
    dbgmsg_v(MSG_DEBUG, "~ Logged-in timer expired ~\n");
    logged_in = false;
}

static void serve_login (FCGX_Request *request, void* arg)
{
    char *data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        if(!user_sign_in(data, app_data.conf.username, app_data.conf.password))
            send_html(request, app_data.conf.login_err_page);
        else{
            send_html(request, app_data.conf.mainmenu_page);
            logged_in = true;
            set_timer(app_data.conf.login_timeout, login_timer_handler);
        }

        free(data);
    }   
}

static void serve_main_content(FCGX_Request *request, void* arg)
{
    char *content = app_get_sensors_data(GET_TEMP_HUM | GET_COOLERS | GET_LIGHT | GET_WATER);

    if(content){
        send_json(request, content);
        free(content);  
    }
}

static void serve_curr_settings(FCGX_Request *request, void* arg)
{
    char *settings = app_get_curr_settings();

    if(settings){
        send_json(request, settings);
        free(settings);  
    }
}


// ---------------- [ FLC ROUTINE ] ----------------
static void serve_flc_update(FCGX_Request *request, void* arg)
{
    char *status = app_get_flc_status();

    if(status){
        send_json(request, status);
        free(status);
    }
}

static void serve_flc_ch_power(FCGX_Request *request, void* arg)
{
    char *data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_set_flc_ch_pwr(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_flc_curr_time(FCGX_Request *request, void* arg)
{
    char *data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_set_flc_time(current, data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_flc_start_time(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_flc_time(start, data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_flc_stop_time(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_flc_time(stop, data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_flc_smooth(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_flc_smooth(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_flc_smooth_value(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_flc_smooth_value(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

// ---------------- [ COOLERS ROUTINE ] ----------------
static void serve_coolers_curr(FCGX_Request *request, void* arg)
{
    char* content = app_get_coolers_state();

    if(content){
        send_json(request, content);
        free(content);  
    }
}

static void serve_coolers_power(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_save_coolers_power(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_coolers_time(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_coolers_time(data, arg);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

// ---------------- [ MAIN BOARD SETTINGS ] ----------------

static void serve_sett_lan(FCGX_Request* request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_set_lan(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data); 
    }
}

static void serve_sett_wifiap_on(FCGX_Request *request, void* arg)
{
    char *result = app_set_wifi_ap(true);
    if(result){
        send_json(request, result);
        free(result);
    }
}

static void serve_sett_wifiap_off(FCGX_Request *request, void* arg)
{
    char *result = app_set_wifi_ap(false);
    if(result){
        send_json(request, result);
        free(result);
    }
}

static void serve_sett_rtc(FCGX_Request *request, void* arg)
{
    char *data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_set_rtc(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_sett_reboot(FCGX_Request *request, void* arg)
{
    char *result = app_reboot();
    if(result){
        send_json(request, result);
        free(result);
    }
}

static void serve_sett_poweroff(FCGX_Request *request, void* arg)
{
    char *result = app_poweroff();
    if(result){
        send_json(request, result);
        free(result);
    }
}

static void serve_sett_restore_dflt(FCGX_Request *request, void* arg)
{
    char *result = app_restore_dflt();
    if(result){
        send_json(request, result);
        free(result);
    }
}

// ---------------- [ CLIMATE REQUESTS ] ----------------

static void serve_set_th_period(FCGX_Request *request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char* result = app_set_th_period(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_get_th_data(FCGX_Request *request, void* arg)
{
    char* result = app_get_th_data(arg);
    if(result){
        send_json(request, result);
        free(result);
    }
}

static void serve_get_th_day_data(FCGX_Request *request, void* arg)
{
    char* data = NULL;

    if ( NULL != (data = http_get_body(request)))
    {
        char *result = app_get_th_day_data(data);
        if(result){
            send_json(request, result);
            free(result);
        }
        free(data);
    }
}

static void serve_avail_th(FCGX_Request *request, void* arg)
{
    char* result = app_get_avail_th_dates();
    if(result){
        send_json(request, result);
        free(result);
    }
}



typedef void (*cb_func_t) (FCGX_Request *request, void* arg);

typedef struct {
    const char* method;
    const char* uri;
    cb_func_t cb;
    void* cb_arg;
}http_req_t;

http_req_t req[] = {
        {"GET", URI_RFU, serve_html, app_data.conf._404_page},
        {"GET", URI_TEMPLATE, serve_html, app_data.conf.template_page},
        {"GET", URI_WELCOME, serve_html, app_data.conf.startup_page},
        {"GET", URI_WELCOME_ERR, serve_html, app_data.conf.startup_page},
        {"POST", URI_LOGIN, serve_login, NULL},
        {"GET", URI_MAIN, serve_html, app_data.conf.startup_page},
        {"GET", URI_MAIN_CONTENT, serve_main_content, NULL},
        {"GET", URI_RETURN_MAIN, serve_html, app_data.conf.mainmenu_page},
        {"GET", URI_SETTINGS, serve_html, app_data.conf.settings_page},
        {"GET", URI_CURR_SETTINGS, serve_curr_settings, NULL},
        {"GET", URI_LIGHT, serve_html, app_data.conf.flc_page},
        {"GET", URI_FLC_UPDATE, serve_flc_update, NULL},
        {"POST", URI_FLC_CURR, serve_flc_curr_time, NULL},
        {"POST", URI_FLC_START, serve_flc_start_time, NULL},
        {"POST", URI_FLC_STOP, serve_flc_stop_time, NULL},
        {"POST", URI_FLC_SMOOTH, serve_flc_smooth, NULL},
        {"POST", URI_FLC_SMOOTH_VAL, serve_flc_smooth_value, NULL},
        {"POST", URI_FLC_CH_PWR, serve_flc_ch_power, NULL},
        {"GET", URI_WATER, serve_html, app_data.conf._404_page},
        {"GET", URI_CLIMATE, serve_html, app_data.conf.temperature_page},
        {"GET", URI_COOLERS, serve_html, app_data.conf.cooler_page},
        {"GET", URI_COOLERS_CURR, serve_coolers_curr, NULL},
        {"POST", URI_COOLERS_POWER, serve_coolers_power, NULL},
        {"POST", URI_COOLERS_ST_IN, serve_coolers_time, CL_START_IN},
        {"POST", URI_COOLERS_STP_IN, serve_coolers_time, CL_STOP_IN},
        {"POST", URI_COOLERS_WP_IN, serve_coolers_time, CL_WP_IN},
        {"POST", URI_COOLERS_ST_OUT, serve_coolers_time, CL_START_OUT},
        {"POST", URI_COOLERS_STP_OUT, serve_coolers_time, CL_STOP_OUT},
        {"POST", URI_COOLERS_WP_OUT, serve_coolers_time, CL_WP_OUT},
        {"POST", URI_SETT_LAN, serve_sett_lan, NULL}, 
        {"POST", URI_SETT_WIFIAP_ON, serve_sett_wifiap_on, NULL},
        {"POST", URI_SETT_WIFIAP_OFF, serve_sett_wifiap_off, NULL},
        {"POST", URI_SETT_RTC, serve_sett_rtc, NULL},
        {"POST", URI_SETT_CPU_REBOOT, serve_sett_reboot, NULL},
        {"POST", URI_SETT_CPU_PWROFF, serve_sett_poweroff, NULL},
        {"POST", URI_SETT_CPU_DFLT, serve_sett_restore_dflt, NULL},
        {"POST", URI_SET_TH_PERIOD, serve_set_th_period, NULL},
        {"GET", URI_LATEST_TH, serve_get_th_data, TH_LATEST},
        {"GET", URI_AVAIL_TH, serve_avail_th, NULL},
        {"POST", URI_DAY_TH, serve_get_th_day_data, NULL},
        {"GET", URI_WEEK_TH, serve_get_th_data, TH_WEEK},
        {"GET", URI_MONTH_TH, serve_get_th_data, TH_MONTH},
    };

size_t rsize = sizeof(req) / sizeof(http_req_t);

// Serving all incomming requests
void httpserv_serve (FCGX_Request *request)
{
    char wifi_serv_ip[64];
    char *req_meth = NULL;
    char *req_uri = NULL;
    uint idx;

    memset(wifi_serv_ip, 0, sizeof(wifi_serv_ip));

    req_meth = FCGX_GetParam("REQUEST_METHOD", request->envp);
    req_uri = FCGX_GetParam("REQUEST_URI", request->envp);

    if(!req_meth || !req_uri) return;

    dbgmsg_v(MSG_VERBOSE, "%s %s\n", req_meth, req_uri);

    if (!strcmp(req_meth, "GET") && !strcmp(req_uri, "/")){
        serve_html(request, app_data.conf.startup_page);
        return;
    }

    // [ HTML ]
    for (idx = 0; idx < rsize; idx++)
    {
        if (!strcmp(req_meth, req[idx].method) && 
            !strncmp(req_uri, req[idx].uri, strlen(req[idx].uri)))
        {
            if(req[idx].cb) req[idx].cb(request, req[idx].cb_arg);
            return;
        }            
    }

    if(strstr(req_uri, ".html")) send_html(request, app_data.conf._404_page);
}

