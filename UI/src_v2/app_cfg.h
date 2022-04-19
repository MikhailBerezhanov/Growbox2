#ifndef _APP_CONFIG_H_ 
#define _APP_CONFIG_H_

#include <cstdint>
#include <string> 
#include <vector> 

namespace app{

// Преобразование МегаБайт в Байты
constexpr uint64_t MB_to_B(uint32_t mbytes) { return mbytes * 1048576; }

struct Settings
{
	// Настройки логирования
	struct log_settings						// Значения по-умолчанию
	{
		std::string log_level_str = "error";
		uint8_t log_level = 1;
		uint64_t log_max_size = MB_to_B(8);
		uint64_t log_backup_max_size = MB_to_B(128);
	}loger;

	// Настройки хранилища
	struct storage_settings
	{
		uint64_t db_max_size = MB_to_B(2);
	}storage;

	// Настройки внешних устройств
	struct devices_settings
	{
		std::string flc_dev = "/dev/ttyUSB0";
		std::string i2c_dev = "/dev/i2c-0";
		std::string lan_iface = "eth0";
		uint coolers_poll_period = 60;
		uint temphum_poll_period = 10;
	}devices;

	// Настройки контроля системы
	struct system_settings
	{
		bool reboot_enabled = false;
		bool poweroff_enabled = false;
	}system;
};


class Config
{
public:

	void init_from_json(const std::string& file_name);

	void init_from_conf(const std::string& file_name);

	//void update();

private:
	Settings settings;
};

}//namespace




#endif


