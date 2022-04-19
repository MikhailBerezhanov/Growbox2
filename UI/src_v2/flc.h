#ifndef _FLC_H_ 
#define _FLC_H_

#include <cstdint>
#include <string>



// Интерфейс контроллера фито-лампы
class FLC
{
public:
	
	typedef enum
	{
		current = 0,
		start,
		stop
	}time_type;

	struct stime
	{
		time_type type = current;
		uint hour = 0;
		uint min = 0;
		uint sec = 0;
	};

	// Мощность каналов контроллера фито-лампы в процентах (0..100 %)
	struct ch_pwr
	{
		// по умолчанию значения не установлены 
		int ch1 = -1;	
		int ch2 = -1;
		int ch3 = -1;
	};


	void init(const std::string& port_path);

	void deinit();

	// Отправка json запроса с получением ответа
	void transcieve(std::string& json_req, std::string& json_resp)
	{
		send_request(json_req);
		get_response(json_resp);
	}

	// Получение текущего статуса контроллера
	void get_status(std::string& status_json)
	{
		std::string json_req = "{}";	// отправка пустого json
		transcieve(json_req, status_json);
	}
	// Установка нового времени контроллера
	void set_time(const stime& st, std::string& result_json);
	// Установка мощности каналов контроллера [%]
	void set_ch_pwr(const ch_pwr& ch_pwr, std::string& result_json);
	// Установка разрешения плавного режима включения лампы
	void set_smooth_ctrl(const bool smooth, std::string& result_json);
	// Установка значения таймера плавности включения лампы
	void set_smooth_value(const uint16_t smooth_value, std::string& result_json);
	
	
private:
	// дескриптор последовательного порта для связи с контроллером
	static int port_fd;
	// Отправка json запроса контроллеру фито-лампы
	void send_request(std::string& json_req);
	// Получение ответа в формате json от контроллера фито-лампы
	void get_response(std::string& json_resp);
};


#endif