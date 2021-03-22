#ifndef _HTTPSERV_H_ 
#define _HTTPSERV_H_

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <fcgios.h>

#include "utils.h"

#define URI_RFU				"/rfu"
#define URI_TEMPLATE		"/template"

#define URI_WELCOME			"/cncu_welcome.html"
#define URI_WELCOME_ERR		"/cncu_welcome_err.hmtl"
#define URI_LOGIN			"/login"
#define URI_MAIN			"/cncu_mainmenu.html"
#define URI_MAIN_CONTENT	"/content"
#define URI_SETTINGS		"/settings"
#define URI_CURR_SETTINGS	"/current_settings"
#define URI_RETURN_MAIN		"/return_mainmenu"
#define URI_LIGHT			"/light_settings"
#define URI_WATER			"/water_status"
#define URI_CLIMATE			"/climate_status"
#define URI_COOLERS			"/coolers_status"

#define URI_COOLERS_CURR	"/coolers/curr_state"
#define URI_COOLERS_POWER	"/coolers/power"
#define URI_COOLERS_ST_IN 	"/coolers/start_in_time"
#define URI_COOLERS_STP_IN 	"/coolers/stop_in_time"
#define URI_COOLERS_WP_IN 	"/coolers/wp_in_time"
#define URI_COOLERS_ST_OUT 	"/coolers/start_out_time"
#define URI_COOLERS_STP_OUT "/coolers/stop_out_time"
#define URI_COOLERS_WP_OUT 	"/coolers/wp_out_time"

#define URI_FLC_UPDATE		"/flc/update"
#define URI_FLC_CURR		"/flc/current_time"
#define URI_FLC_START		"/flc/start_time"
#define URI_FLC_STOP		"/flc/stop_time"
#define URI_FLC_SMOOTH		"/flc/smooth_control"
#define URI_FLC_SMOOTH_VAL	"/flc/smooth_value"
#define URI_FLC_CH_PWR		"/flc/ch_power"

#define URI_SETT_LAN		"/settings/lan"
#define URI_SETT_WIFIAP_ON	"/settings/wifi_ap_state?on"
#define URI_SETT_WIFIAP_OFF	"/settings/wifi_ap_state?off"
#define URI_SETT_RTC		"/settings/rtc"
#define URI_SETT_ACC		"/settings/acc"
#define URI_SETT_CPU_REBOOT	"/settings/cpu/reboot"
#define URI_SETT_CPU_PWROFF	"/settings/cpu/poweroff"
#define URI_SETT_CPU_DFLT	"/settings/cpu/restore_settings"

#define URI_SET_TH_PERIOD	"/climate/set_temp_hum_period"
#define URI_LATEST_TH		"/climate/latest_temp_hum"
#define URI_AVAIL_TH		"/climate/avail_dates"
#define URI_DAY_TH			"/climate/day_temp_hum"
#define URI_WEEK_TH			"/climate/week_temp_hum"
#define URI_MONTH_TH		"/climate/month_temp_hum"

// HTTP Запрос от библиотеки fcgi
using req_t = FCGX_Request;

// Форма обработчика HTTP запроса
struct Req_handler
{
	// Колбек функция ответа на запрос
	using cb_func = void (*) (req_t* , void* );

	Req_handler(const char* m, const char* u, cb_func func, void* func_arg): 
		method(m), uri(u), cb(func), cb_arg(func_arg) { }

	Req_handler(const char* m, const char* u, cb_func func, const void* func_arg): 
		Req_handler(m, u, func, const_cast<void*>(func_arg)) { }

	Req_handler(const char* m, const char* u, cb_func func):
		method(m), uri(u), cb(func), cb_arg(nullptr) { }
	
	Req_handler(char* m, char* u, cb_func func):
		Req_handler(const_cast<const char*>(m), const_cast<const char*>(u), func) { }

	Req_handler(const char* m, const char* u):
		method(m), uri(u), cb(nullptr), cb_arg(nullptr) { }
		
	Req_handler(char* m, char* u):
		Req_handler(const_cast<const char*>(m), const_cast<const char*>(u)) { }

	const char* method;		// HTTP-метод запроса
    const char* uri;		// URI-адрес запроса
    cb_func cb;				// Колбек функция обработки запроса
    void* cb_arg;			// Аргумент колбек функции обработки запроса
};

// Перегрузка для использования в стандартных алгоритмах (find)
bool operator== (const Req_handler& lhs, const Req_handler& rhs);
bool operator!= (const Req_handler& lhs, const Req_handler& rhs);


// Класс обработки поступающих HTTP запросов от прокси-сервера (NGINX) через FCGI
class HTTP_serv
{
public:

	HTTP_serv() = default;
	HTTP_serv(std::initializer_list<Req_handler> list): req_vec(list) { }

	// Добавление в вектор обработчиков запросов новых элементов
	void add_req_handlers(std::initializer_list<Req_handler> list) 
	{
		for(const auto req_handler : list){
			req_vec.push_back(req_handler);
		}
	}

	// Инициализация библиотеки FCGI и сокета для связи с прокси-сервером (NGINX)
	void init(const std::string& sock_path, const int backlog)
	{
	    FCGX_Init(); 
	    fcgi_socket_fd = FCGX_OpenSocket(sock_path.c_str(), backlog);
	    if(fcgi_socket_fd < 0) throw std::runtime_error("fcgi socket (" + sock_path + ") opening failed");
	    // Установка прав доступа к сокету для разрешения чтения и записи извне (nginx)
	    utils::change_mod(sock_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	}

	// Деинициализация библиотеки FCGI и сброс соединения с прокси-сервером
	// ПРИМ: также сбрасывает все текущие блокирубщие вызовы accept_request()
	void deinit()
	{
		FCGX_ShutdownPending();
	    if(fcgi_socket_fd >= 0) {
	        shutdown(fcgi_socket_fd, SHUT_RD);
	        OS_IpcClose(fcgi_socket_fd);
	    }
	    OS_LibShutdown();
	}

	// Инициализация буфера для хранения данных HTTP-запроса
	req_t request_init()
	{
		req_t request;
	    if(FCGX_InitRequest(&request, fcgi_socket_fd, FCGI_FAIL_ACCEPT_ON_INTR)) { 
	        throw std::runtime_error("FCGX_InitRequest() failed\n"); 
	    }

	    return request;
	}

	// Блокирующее ожидание поступления очередного HTTP-запроса от прокси-сервера
	int accept_request(req_t* req)
	{
		return FCGX_Accept_r(req);
	}

	// Обслуживание поступившего запроса при его наличии в векторе req_vec
	void serve(req_t* req);

private:
	int fcgi_socket_fd = -1;			// Файловый дестриптор сокета FCGI <-> NGINX
	std::vector<Req_handler> req_vec;	// Вектор обработчиков HTTP запросов
};


// Вспомогательные ф-ии для разработки колбеков обработки запросов

// Получить строку - тело запроса
void get_request_body(req_t* req, std::string& body);

// Отправить HTML-файл в качестве ответа
void response_with_HTML(req_t* req, const std::string& file_name);

// Отправить JSON строку в качестве ответа
void response_with_JSON(req_t* req, const std::string& json);


#endif
