
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cinttypes>
#include <string> 
#include <array>
#include <stdexcept>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h> 
#include <sys/select.h>

#include "flc.h"

#define LOG_MODULE_NAME		"[ FLC ] "
#include "loger.h"

int FLC::port_fd = -1;


void FLC::init (const std::string& port_path)
{
	struct termios options;
	memset(&options, 0, sizeof(struct termios));

	port_fd = open(port_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	if (port_fd < 0){
		throw std::runtime_error(excp_msg("Unable to open port '" + port_path + "': " + strerror(errno)));
	}
	
	fcntl(port_fd, F_SETFL, 0);

	tcgetattr(port_fd, &options);
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	options.c_cflag |= (CLOCAL | CREAD);
#if 0
	// RAW input
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	// RAW output
	options.c_oflag &= ~OPOST;
#else
	// Сырой поток данных
	cfmakeraw(&options);
#endif

	// 1-секундный таймаут
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 10;

	// Отсутствие проверки на четность (8N1)
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	if(tcsetattr(port_fd, TCSANOW, &options) < 0){
    	throw std::runtime_error(excp_msg((std::string)"Unable to set port parameters: " + strerror(errno)));   
	}

	tcflush(port_fd, TCIOFLUSH);
}

void FLC::deinit()
{
	close(port_fd);
}


void FLC::send_request(std::string& json_req)
{
	tcflush(port_fd, TCIOFLUSH);

	// Add terminating symbols
	json_req += "\r\n";
	log_msg(MSG_TRACE, "Sending request: %s\n", json_req);
	std::string::size_type req_len = json_req.length();

	if (write(port_fd, json_req.c_str(), req_len) < req_len){
		throw std::runtime_error(excp_msg((std::string)"write() failed: " + strerror(errno)));
	}
}

void FLC::get_response(std::string& json_resp)
{
	std::array<char, 128> buf{};
	int res = 0;
	fd_set fs;
	struct timeval timeout;

   	timeout.tv_sec  = 2;
    timeout.tv_usec = 0;
    FD_ZERO(&fs);
    FD_SET(port_fd, &fs);

    res = select( port_fd+1, &fs, NULL, NULL, &timeout );
    if(res < 0) throw std::runtime_error(excp_msg((std::string)"select() failed: " + strerror(errno)));

    if ( (res > 0) && (FD_ISSET(port_fd, &fs)) ) 
    {
    	json_resp = "";
    	std::string::size_type len = 0;
    	res = 0;

    	while(1){
    		std::fill(begin(buf), end(buf), 0);
    		res = read(port_fd, buf.data(), buf.size()-1);
        	log_msg(MSG_OVERTRACE, "Read %d bytes: %s\n", res, buf.data());

        	if(!res) throw std::runtime_error(excp_msg("Timeout waiting End of response"));

        	json_resp += buf.data();

        	len = json_resp.length();
        	log_msg(MSG_OVERTRACE, "response length: %zu\n", len);
        	if( len && json_resp[len - 2] == '\r' && json_resp[len - 1] == '\n' )
        	{
        		// Удалить стоп-символы из результата
        		json_resp.pop_back();
        		json_resp.pop_back();
        		break;
        	} 	
    	}

    	log_msg(MSG_VERBOSE, "Got response:\n%s\n", json_resp);
	}
	else {
		throw std::runtime_error(excp_msg((std::string)"Waiting response timeout: " + strerror(errno)));
	}
}


void FLC::set_ch_pwr(const ch_pwr& ch_pwr, std::string& result_json)
{
	// Формирование json запроса
	std::string json_req = "{";
	if(ch_pwr.ch1 >= 0) json_req += "\"ch1_power\":" + std::to_string(ch_pwr.ch1);
	if(ch_pwr.ch2 >= 0) json_req += "\"ch2_power\":" + std::to_string(ch_pwr.ch2);
	if(ch_pwr.ch3 >= 0) json_req += "\"ch3_power\":" + std::to_string(ch_pwr.ch3);
	json_req += "}"; 

	if (json_req == "{}") throw std::runtime_error(excp_msg("No channel-power value provided"));

	transcieve(json_req, result_json);
}

void FLC::set_time(const stime& st, std::string& result_json)
{
	std::string json_req;
	std::array<char, 64> buf{}; // инициализируется нулями
	sprintf(buf.data(), "\"%02u:%02u:%02u\"}", st.hour, st.min, st.sec);

	switch(st.type)
	{
		case current: json_req = "{\"current_time\":"; break;
		case start: json_req = "{\"start_time\":"; break;
		case stop: json_req = "{\"stop_time\":"; break;

		default: throw std::runtime_error(excp_msg("Unsupported time type (" + std::to_string(st.type) +") provided"));
	}
	json_req += buf.data();

	transcieve(json_req, result_json);
}

void FLC::set_smooth_ctrl(const bool smooth, std::string& result_json)
{
	std::string json_req = (std::string)"{\"smooth_control\":}" + (smooth ? "true" : "false") + "}";
	transcieve(json_req, result_json);
}

void FLC::set_smooth_value(const uint16_t smooth_value, std::string& result_json)
{
	std::string json_req = "{\"smooth_value\":" + std::to_string(smooth_value) + "}";
	transcieve(json_req, result_json);
}


#ifdef UNIT_TEST

int main(int argc, char* argv[])
{
	FLC flc;

	try{
		Log::init(MSG_TRACE, "Log.log", 1000);

		flc.init("/dev/ttyUSB0");

		log_msg(MSG_DEBUG, "--- Sending status request\n");
		std::string result;
		flc.get_status(result);

		log_msg(MSG_DEBUG, "--- Sending set_time request\n");
		FLC::stime new_start_time = {FLC::start, 10, 20, 30};
		flc.set_time(new_start_time, result);

		log_msg(MSG_DEBUG, "--- Sending set_ch_pwr request\n");
		FLC::ch_pwr new_pwr;
		new_pwr.ch2 = 40;
		flc.set_ch_pwr(new_pwr, result);
	}
	catch(const std::runtime_error& e){
		log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
	}

	flc.deinit();

	return 0;
}

#endif