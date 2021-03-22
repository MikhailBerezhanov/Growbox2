#ifndef _UTILS_H
#define _UTILS_H

#include <ctime>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <sys/stat.h>


namespace utils
{
// 
typedef struct ipv4{
	ipv4() { memset(this, 0, sizeof *this); }

	std::string as_str;
	char as_ch[4][4];
	uint8_t as_byte[4];
}ipv4_t;

// Выполнить команду в оболочке и считать вывод через пайп в строку
std::string exec(const std::string& cmd);

// --- Операции с системным временем
// Получить текущее время системы в виде строки
void get_local_time(std::string& s);
// Получить текущее время системы в виде структуры
void get_local_time(struct tm* result);
// Установка системного времени
void set_time(const std::string& hour, const std::string& min, const std::string& sec);
// Установка системной даты
void set_date(const std::string& mday, const std::string& month, const std::string& year);

// --- Операции с файловой системой
// Получение текущего абсолютного пути рабочей директории
void get_cwd(std::string& s);
// Создание новой директории если таковой еще не существует
void make_new_dir(const std::string& path, mode_t mode);
// Изменение прав доступа к файлу
void change_mod(const std::string& path, mode_t mode);
// Получение кол-ва файлов в директории
uint32_t get_files_num_in_dir(const std::string& dir_name);
// Получение имен файлов в директории
void get_file_names_in_dir(const std::string& dir_name, std::vector<std::string>& vec);

// --- Операции с файлами
// Получение размера файла в байтах
uint64_t get_file_size (const std::string& fname);
// Запись в бинарный файл
void write_bin_file(const std::string& fname, const uint8_t* buf, size_t buf_len);
// Чтение бинарного файла
std::unique_ptr<uint8_t[]> read_bin_file(const std::string& fname, uint64_t* buf_len);
// Запись в текстовый файл потока символов
void write_text_file(const std::string& fname, const char* buf, size_t buf_len);
// Чтение текстового файла в вектор строк
void read_text_file(const std::string& fname, std::vector<std::string>& vec);
// Чтение текстового файла в динамическую память
std::unique_ptr<char[]> read_text_file(const std::string& fname);

// --- Операции с сетевыми настройками
// Получение имени соединения из имени интерфейса
std::string get_connection_name(const std::string& iface_name);
// Получение текущего IP адреса системы
void get_current_ip (ipv4_t& res);
void get_current_ip (const std::string& iface_name, ipv4_t& ip);
// Получение текущего адреса сетевого шлюза
void get_current_gateway(const std::string& iface_name, ipv4_t& gw);
// Получение статуса DHCP
bool get_current_dhcp_state(const std::string& iface_name);
// Установка динамического IP адреса (DHCP)
void set_dynamic_ip(const std::string& iface_name);
// Установка статического IP адреса
void set_static_ip(const std::string& iface_name, const std::string& ip, const std::string& gw);
// Получение статуса точки доступа WiFi
bool get_wifi_ap_state();
// Установка точки доступа WiFi при помощи скрипта 
void set_wifi_ap_state(bool on_off);

// --- Контроль системы
// Получение температуры ЦПУ системы
void get_cpu_temp(std::string& cpu_temp);
// Перезагрузка системы
void system_reboot();
// Выключение системы
void system_poweroff();

}



#endif