#ifndef _SENSORS_H_ 
#define _SENSORS_H_

#include <stdint.h>
#include <stdlib.h>

#if APP_MULTITHREADED
	#define SENS_MUTUAL	1			// Mutual operations with sensors
#else 
	#define SENS_MUTUAL	0
#endif

typedef enum{
	input,
	output,
}cooler_t;

#define SENS_OK		0

int sensors_init(char* i2c_path);
void sensors_deinit(void);

int sht30_read_data(float* temperature, float* humidity);

int set_coolers_power(cooler_t name, uint8_t power);
int get_coolers_power(cooler_t name, uint8_t* power);

int get_water_sensor(char* ws_path, uint* value);

#endif