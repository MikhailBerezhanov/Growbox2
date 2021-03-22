#include <algorithm>

#include "httpserv.h"

#define LOG_MODULE_NAME   "[ HTTPS ] "
#include "loger.h"



bool operator== (const Req_handler& lhs, const Req_handler& rhs)
{
    if (!strcmp(lhs.method, rhs.method) && !strcmp(lhs.uri, rhs.uri)) {
        return true;
    }
    return false;
}

bool operator!= (const Req_handler& lhs, const Req_handler& rhs)
{
    return !(lhs == rhs);
}

// Обслуживание поступившего запроса при его наличии в векторе
void HTTP_serv::serve(req_t* req)
{
    char* req_meth = FCGX_GetParam("REQUEST_METHOD", req->envp);
    char* req_uri = FCGX_GetParam("REQUEST_URI", req->envp);

    if(!req_meth || !req_uri) throw std::runtime_error(excp_msg("Failed to get HTTP Method or URI"));

    log_msg(MSG_VERBOSE, "Request: %s %s\n", req_meth, req_uri);

    // Поиск соответствующего обработчика запроса
    std::vector<Req_handler>::iterator it;
    it = find(req_vec.begin(), req_vec.end(), Req_handler(req_meth, req_uri));

    if(it != req_vec.end() && it->cb){
        // Вызов колбек функции - обработчика запроса
        it->cb(req, it->cb_arg);
    }
    else{
        log_msg(MSG_DEBUG, "No handler found for request: %s %s\n", req_meth, req_uri);
    }

    //if(strstr(req_uri, ".html")) send_html(request, app_data.conf._404_page);
}


 /*
  *----------------------------------------------------------------------
  *
  * FCGX_GetStr --
  *
  *      Reads up to n consecutive bytes from the input stream
  *      into the character array str.  Performs no interpretation
  *      of the input bytes.
  *
  * Results:
  *  Number of bytes read.  If result is smaller than n,
  *      the end of input has been reached.
  *
  *----------------------------------------------------------------------
  */
 //int FCGX_GetStr(char *str, int n, FCGX_Stream *stream)
 
// Получить строку - тело запроса
void get_request_body(req_t* req, std::string& body)
{
    char* content_len = FCGX_GetParam("CONTENT_LENGTH", req->envp);
    char* content_type = FCGX_GetParam("CONTENT_TYPE", req->envp);
    std::array<char, 512> buf{};

    if (!content_type || !content_len) throw std::runtime_error(excp_msg("CONTENT_LENGTH or CONTENT_TYPE is empty\n"));

#if 0
    int ch = -1;
    while((ch = FCGX_GetChar(req->in)) != -1){
        body += static_cast<char>(ch);
    }
#else
    for(int len = buf.size(); len >= buf.size(); ){
        std::fill(begin(buf), end(buf), 0);
        len = FCGX_GetStr(buf.data(), buf.size(), req->in);
        body += buf.data();
    }
#endif

    log_msg(MSG_TRACE, "HTTP data:\n%s\n", body);  
}

// Отправить HTML-файл в качестве ответа
void response_with_HTML(req_t* req, const std::string& file_name)
{
    try{
        log_msg(MSG_TRACE, "Sending '%s'\n", file_name);

        std::unique_ptr<char[]> content = utils::read_text_file(file_name);

        FCGX_PutS("Content-type: text/html\r\n\r\n", req->out);
        FCGX_PutS(content.get(), req->out);
        FCGX_PutS("\r\n", req->out);
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }

    FCGX_Finish_r(req);
}

// Отправить JSON строку в качестве ответа
void response_with_JSON(req_t* req, const std::string& json)
{
    FCGX_PutS("Content-type: application/json\r\n\r\n", req->out);
    FCGX_PutS(json.c_str(), req->out);
    FCGX_PutS("\r\n", req->out);

    FCGX_Finish_r(req);
}


#ifdef UNIT_TEST

#include <csignal>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <unistd.h>

#define NUM_THREAD    4

static std::atomic<int> stop_main;
static std::mutex stop_threads_mutex;
static std::condition_variable stop_threads;
static std::mutex request_mutex;
static HTTP_serv hs;

static void test_handler (req_t* req, void* arg)
{
    try{
        char* file_name = reinterpret_cast<char*>(arg);

        response_with_HTML(req, file_name);
    
        log_msg(MSG_DEBUG, "Going to sleep...\n");
        sleep(10);
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }
}


static void serve_login (req_t* req, void* arg)
{
    try{
        std::string data;
        get_request_body(req, data);
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }
}


static void worker_thread(int tid)
{
    log_msg(MSG_DEBUG, "thread: %d starts\n", tid);

    try{
        int ret = 0;
        req_t request = hs.request_init();

        for(;;)
        {
            request_mutex.lock();
            ret = hs.accept_request(&request);
            request_mutex.unlock();

            if (ret < 0){
                log_err("accept_request() failed\n");
                break;
            }
        
            hs.serve(&request);

            std::unique_lock<std::mutex> lock(stop_threads_mutex);
            if(stop_threads.wait_for(lock, std::chrono::milliseconds(5250)) == std::cv_status::no_timeout){
                log_msg(MSG_DEBUG, "thread: %d interrupted\n", tid);
                break;
            } 
        }
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }
    catch(...){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "Unknown exception occured in thread %d\n", tid);
    }
    
}


static inline void signal_handler_init(std::initializer_list<int> signals)
{
    stop_main.store(0);

    auto signal_handler = [](int sig_num){ stop_main.store(sig_num); };

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;        
    sigemptyset(&act.sa_mask);     

    for (const auto sig : signals){
        sigaddset(&act.sa_mask, sig);
        sigaction(sig, &act, nullptr);
    }                                        
}

int main(int argc, char* argv[]) 
{ 
    try{
        Log::init(MSG_TRACE, "Log.log", 1000);

        signal_handler_init({SIGINT, SIGQUIT, SIGTERM, SIGHUP});

        hs.init("/home/mik/projects/gb_web/gbweb.sock", 20);
        hs.add_req_handlers({ 
            Req_handler("GET", "/", test_handler, "/home/mik/projects/gb_web/data/gb_startup.html"),
            Req_handler("POST", URI_LOGIN, serve_login), 
        });



        std::thread t[NUM_THREAD];
        for(int i = 0; i < NUM_THREAD; ++i){
            t[i] = std::thread(worker_thread, i);
        }

        while(!stop_main.load()){

            sleep(10); // Сигналы прерывают сон
        }

        log_msg(MSG_DEBUG | MSG_TO_FILE, "catched signal: %d\n", stop_main.load());

        hs.deinit();

        stop_threads.notify_all();

        for(int i = 0; i < NUM_THREAD; ++i){
            t[i].join();
        }

        log_msg(MSG_DEBUG, "all threads stopped. exiting\n");

        return 0;      
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
        return 1;
    }
    catch(...){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "Unknown exception occured\n");
        return 2;
    }
}

#endif





#if 0

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
#endif

