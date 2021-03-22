#include <memory>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <libconfig.h++>

#include "utils.h"
#include "json.hpp"
#include "app_cfg.h"

#define LOG_MODULE_NAME		"[ CONF ] "
#include "loger.h"


namespace app{

#define CHECK_AND_SET_J(root, name, dest)	do{ \
	if(root.contains(name)){ \
		dest = root[name]; \
		if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Setting " << _BOLD << name << _RESET << " to: " << dest << std::endl; \
	} \
	else if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Using default " << _BOLD << name << _RESET << ": " << dest << std::endl; \
}while(0)


static log_lvl_t log_str_to_lvl(std::string& str)
{
	// 
	for(auto& c: str) c = toupper(c);

	if(str == "SILENT") return MSG_SILENT;
	if(str == "DEBUG") return MSG_DEBUG;
	if(str == "ERROR") return MSG_ERROR;
	if(str == "VERBOSE") return MSG_VERBOSE;
	if(str == "TRACE") return MSG_TRACE;
	if(str == "OVERTRACE") return MSG_OVERTRACE;

	log_err("'%s' is unsupported log level. Using default instead\n", str);
	return LOG_LVL_DEFAULT;
}


void Config::init_from_json(const std::string& file_name)
{
	using json = nlohmann::json;

	std::unique_ptr<char[]> json_str = utils::read_text_file(file_name);

	auto cfg = json::parse(json_str.get());

	std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Config file json:\n" << std::setw(4) << cfg << std::endl;

	// Настройки логирования
	if (cfg.contains("log_settings")){
		auto loger = cfg["log_settings"];

		CHECK_AND_SET_J(loger, "log_level", settings.loger.log_level_str);
		CHECK_AND_SET_J(loger, "log_max_size", settings.loger.log_max_size);
		CHECK_AND_SET_J(loger, "log_backup_max_size", settings.loger.log_backup_max_size);
	}

	// Настройки хранилища
	if (cfg.contains("storage_settings")){
		auto storage = cfg["storage_settings"];

		CHECK_AND_SET_J(storage, "db_max_size", settings.storage.db_max_size);
	}

	// Настройки внешних устройств
	if (cfg.contains("devices_settings")){
		auto devices = cfg["devices_settings"];

		CHECK_AND_SET_J(devices, "flc_dev", settings.devices.flc_dev);
		CHECK_AND_SET_J(devices, "i2c_dev", settings.devices.i2c_dev);
		CHECK_AND_SET_J(devices, "lan_iface", settings.devices.lan_iface);
		CHECK_AND_SET_J(devices, "coolers_poll_period", settings.devices.coolers_poll_period);
		CHECK_AND_SET_J(devices, "temphum_poll_period", settings.devices.temphum_poll_period);
	}

	// Настройки контроля системы
	if (cfg.contains("system_settings")){
		auto system = cfg["system_settings"];

		CHECK_AND_SET_J(system, "reboot_enabled", settings.system.reboot_enabled);
		CHECK_AND_SET_J(system, "poweroff_enabled", settings.system.poweroff_enabled);
	}

	// Настройки 
	if (cfg.contains("misc_settings")){

	}
}


#define CHECK_AND_SET(root, name, dest)	do{ \
	if(root.lookupValue(name, dest)){ \
		if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Setting " << _BOLD << name << _RESET << " to: " << dest << std::endl; \
	} \
	else if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Using default " << _BOLD << name << _RESET << ": " << dest << std::endl; \
}while(0)

#define CHECK_AND_SET_MB(root, name, dest)	do{ \
	uint tmp = 0; \
	if(root.lookupValue(name, tmp)){ \
		dest = MB_to_B(tmp); \
		if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Setting " << _BOLD << name << _RESET << " to: " << dest <<  " Bytes" << std::endl; \
	} \
	else if(Log::check_lvl(MSG_VERBOSE)) std::cout << Log::make_msg_stamp(Log::dtime_stamp, LOG_MODULE_NAME) << "Using default " << _BOLD << name << _RESET << ": " << dest <<  " Bytes" << std::endl; \
}while(0)

void Config::init_from_conf(const std::string& file_name)
{
	libconfig::Config cfg;

	cfg.readFile(file_name.c_str());
	// catch(const libconfig::FileIOException &fioex)
	// catch(const libconfig::ParseException &pex)

	const libconfig::Setting& root = cfg.getRoot();

	try{
		const libconfig::Setting& loger = root["application"]["log_settings"];

		CHECK_AND_SET(loger, "log_level", settings.loger.log_level_str);
		settings.loger.log_level = log_str_to_lvl(settings.loger.log_level_str);
		CHECK_AND_SET_MB(loger, "log_max_size", settings.loger.log_max_size);
		CHECK_AND_SET_MB(loger, "log_backup_max_size", settings.loger.log_backup_max_size);
	}
	catch(const libconfig::SettingNotFoundException &e){
	    log_msg(MSG_VERBOSE, "%s, Using default 'log_settings'\n", e.what());
	}

	try{
		const libconfig::Setting& storage = root["application"]["storage_settings"];

		CHECK_AND_SET_MB(storage, "db_max_size", settings.storage.db_max_size);
	}
	catch(const libconfig::SettingNotFoundException &e){
	    log_msg(MSG_VERBOSE, "%s, Using default 'storage_settings'\n", e.what());
	}

	try{
		const libconfig::Setting& devices = root["application"]["devices_settings"];

		CHECK_AND_SET(devices, "flc_dev", settings.devices.flc_dev);
		CHECK_AND_SET(devices, "i2c_dev", settings.devices.i2c_dev);
		CHECK_AND_SET(devices, "lan_iface", settings.devices.lan_iface);
		CHECK_AND_SET(devices, "coolers_poll_period", settings.devices.coolers_poll_period);
		CHECK_AND_SET(devices, "temphum_poll_period", settings.devices.temphum_poll_period);
	}
	catch(const libconfig::SettingNotFoundException &e){
	    log_msg(MSG_VERBOSE, "%s, Using default 'devices_settings'\n", e.what());
	}

	try{
		const libconfig::Setting& system = root["application"]["system_settings"];

		CHECK_AND_SET(system, "reboot_enabled", settings.system.reboot_enabled);
		CHECK_AND_SET(system, "poweroff_enabled", settings.system.poweroff_enabled);
	}
	catch(const libconfig::SettingNotFoundException &e){
	    log_msg(MSG_VERBOSE, "%s, Using default 'system_settings'\n", e.what());
	}
	
}

}//namespace

#ifdef UNIT_TEST
int main(int argc, char* argv[])
{
	try{
		Log::init(MSG_VERBOSE, "Log.log", 100000);

		log_msg(MSG_DEBUG, "CONF starts\n");

		app::Config cfg;
		cfg.init_from_json("./gbweb.config.json");
		cfg.init_from_conf("./gbweb.config");
	}
	catch(const std::runtime_error& e){
		log_err("%s\n", e.what());
	}
	catch(const std::exception& e){	
		log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
		// libconfig::FileIOException
		// libconfig::ParseException 
	}
	catch(...){
		printf("unknown error\n");
	}

	return 0;
}
#endif