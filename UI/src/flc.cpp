
#include <stdio.h>
#include <string.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <stdlib.h>

#include "flc.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ FLC ] "
#include "dbgmsg.h"

static int flc_fd = -1;




int flc_init (const char* tty_name)
{
	struct termios options;
	memset(&options, 0, sizeof(struct termios));

	flc_fd = open(tty_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (flc_fd == -1){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to open port '%s': %s\n", tty_name, strerror(errno));
		return -1;
	}
	else fcntl(flc_fd, F_SETFL, 0);

	tcgetattr(flc_fd, &options);
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	options.c_cflag |= (CLOCAL | CREAD);
	// RAW input
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	// RAW output
	options.c_oflag &= ~OPOST;

	// 1-секундный таймаут
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 10;

	// Отсутствие проверки на четность (8N1)
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	if(tcsetattr(flc_fd, TCSANOW, &options) < 0){
    	errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to set port parameters: %s\n", strerror(errno));   
    	return -2;  
	}

	if(tcflush(flc_fd, TCIOFLUSH) < 0){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to flush port: %s\n", strerror(errno));
	}

	return 0;
}

void flc_deinit(void)
{
	close(flc_fd);
}


static int flc_send_request(const char* req, ssize_t req_len)
{
	tcflush(flc_fd, TCIOFLUSH);

	if (write(flc_fd, req, req_len) < req_len){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "write failed: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static int flc_get_responce(char* resp, ssize_t resp_size)
{
	char *buf_ptr = resp;
	memset(buf_ptr, 0x00, resp_size);
	int res = 0;
	fd_set fs;
	struct timeval timeout;

   	timeout.tv_sec  = 2;
    timeout.tv_usec = 0;
    FD_ZERO(&fs);
    FD_SET(flc_fd, &fs);

    res = select( flc_fd+1 , &fs, NULL, NULL, &timeout );
    if(res < 0){
    	errmsg_v(MSG_DEBUG | MSG_TO_FILE, "select failed: %s\n", strerror(errno));
    }
    else if ( (res > 0) && (FD_ISSET(flc_fd, &fs)) ) 
    {
    	res = 0;

    	while(1){
    		res = read(flc_fd, buf_ptr, resp_size - res - 1);
        	//dbgmsg("Read %d bytes: ", res);
        	//print_arr("", (unsigned char *)buf_ptr, res);

        	if(!res){
        		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Timeout waiting End of response\n");
        		break;
        	}

        	if(buf_ptr[res-1] == '\n') break;
        	buf_ptr += res;
    	}

    	dbgmsg_v(MSG_VERBOSE, "FLC response:\n%s", resp);
        
	}
	else {
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Waiting response timeout: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

#define FLC_TRIES_NUM 2

int flc_get_status(flc_resp_t *status)
{
	char buf[512];
	jerr_t jerr;

	for (int i = 0; i < FLC_TRIES_NUM; i++){
		if(flc_send_request("{}\r\n", 4) < 0) return -1;

		if(flc_get_responce(buf, sizeof(buf)) < 0) return -2;
	    
		if((jerr = json_to_flc_resp(buf, status)) != J_OK){
			errmsg_v(MSG_DEBUG | MSG_TO_FILE, "json_to_flc_resp() failed (%s)\n", jsont_get_err_text(jerr));
			if(jerr != J_PARSER_ERR) return jerr; 
		}
		else break; 
	}

	return 0;
}


static res_t flc_transcieve(flc_req_t *req)
{
	char buf[512];
	char* json = NULL;
	flc_resp_t status;
	res_t res;
	jerr_t jerr;
	res.code = 13;
	strcpy(res.text, "flc_transcieve() failed");

	if (NULL == (json = flc_req_to_json(req))){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "flc_req_to_json() failed\n");
		return res;
	}

	ssize_t new_size = strlen(json)+3;

	if(NULL== (json = (char*)realloc(json, new_size))){
		errmsg_v(MSG_DEBUG | MSG_TO_FILE, "realloc failed\n");
		goto exit;
	}

	json[new_size-3] = '\r';
	json[new_size-2] = '\n';
	json[new_size-1] = '\0';

	dbgmsg_v(MSG_TRACE, "Sending request: %s\n", json);
	for (int i = 0; i < FLC_TRIES_NUM; i++){
		if(flc_send_request(json, strlen(json)) < 0) goto exit;

		if(flc_get_responce(buf, sizeof(buf)) < 0) goto exit;

		if((jerr = json_to_flc_resp(buf, &status)) != J_OK) {
			errmsg_v(MSG_DEBUG | MSG_TO_FILE, "json_to_flc_resp() failed (%s)\n", jsont_get_err_text(jerr));
			if(jerr != J_PARSER_ERR) goto exit;
		}
		else break;
	}

	res.code = status.res.code;
	memset(res.text, 0, sizeof(res.text));
	strcpy(res.text, status.res.text);

exit:
	if(json) free(json);
	return res;
}

res_t flc_set_chpwr(flc_ch_pwr_t *ch_pwr)
{
	flc_req_t req;
	
	req.ch_pwr.ch1 = ch_pwr->ch1;
	req.ch_pwr.ch2 = ch_pwr->ch2;
	req.ch_pwr.ch3 = ch_pwr->ch3;

	dbgmsg_v(MSG_TRACE, "new ch1_power: %d, ch2_power: %d, ch3_power: %d\n", req.ch_pwr.ch1, req.ch_pwr.ch2, req.ch_pwr.ch3);

	return flc_transcieve(&req);
}

res_t flc_set_time(flc_time_t type, my_time_t *tm)
{
	flc_req_t req;

	switch(type)
	{
		case current: sprintf(req.curr_time, "%02u:%02u:%02u", tm->hour, tm->min, tm->sec); break;
		case start: sprintf(req.start_time, "%02u:%02u:%02u", tm->hour, tm->min, tm->sec); break;
		case stop: sprintf(req.stop_time, "%02u:%02u:%02u", tm->hour, tm->min, tm->sec); break;
	}

	return flc_transcieve(&req);
}

res_t flc_set_smooth_ctrl(bool smooth)
{
	flc_req_t req;

	req.smooth = smooth ? 1 : 0;

	return flc_transcieve(&req);
}

res_t flc_set_smooth_value(int smooth_value)
{
	flc_req_t req;
	// FLC uses 16-bit timer
	req.smooth_value = (smooth_value > UINT16_MAX ) ? UINT16_MAX  : smooth_value;

	return flc_transcieve(&req);
}