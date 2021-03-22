#ifndef _SENSORS_H_ 
#define _SENSORS_H_

#include <cstdint>
#include <mutex>


#define I2C_ADAPTER     	"/dev/i2c-0"

#define SHT30_ADDR			(0x44)
#define SENSORS_BOARD_ADDR	(0x43)

class Sensors
{
public:

	Sensors(const char* i2c_dev = I2C_ADAPTER) { init(i2c_dev); } 
	~Sensors() { deinit(); }

	enum cooler_type
	{
		input,
		output,
	};

	void init(const char* i2c_dev = I2C_ADAPTER);

	void deinit();

	void SHT30_read(float& temperature, float& humidity);

	void set_coolers_power(cooler_type name, const uint8_t power);
	void get_coolers_power(cooler_type name, uint8_t& power);

	bool get_water_sensor();

private:
	static int i2c_fd;
	static std::mutex i2c_mutex;
	static std::mutex water_sensor_mutex;
};

#endif