#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <fstream>
#include <string>
#include <stdexcept>
#include <mutex>
#include <memory>
#include <vector>
#include <array>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include "utils.h"

#define LOG_MODULE_NAME   "[ UTILS ] "
#include "loger.h"

namespace utils
{

/**
  * @описание   Выполнить команду в оболочке и считать вывод через пайп в строку
  * @параметры
  *     Входные:
  *         cmd - строка, содержащая команду оболочки среды
  * @возвращает Строку с результатом выполнения команды
 */
std::string exec(const std::string& cmd)
{
    std::array<char, 128> buf{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if(!pipe) throw std::runtime_error(excp_msg((std::string)"popen() failed: " + strerror(errno)));

    while(fgets(buf.data(), buf.size(), pipe.get()) != nullptr){
        result += buf.data();
        std::fill(begin(buf), end(buf), 0);
    }

    return result;
}

/**
  * @описание   Получить текущее время системы в виде строки
  * @параметры
  *     Выходные:
  *         s - строка, содержащая системное время
 */
void get_local_time(std::string& s)
{
    time_t seconds = time(NULL);
    struct tm timeinfo;
 
    if(!localtime_r(&seconds, &timeinfo)){
        s = "localtime_r failed";
        return;
    }

    char buf[64];
    strftime(buf, sizeof buf, "%d.%m.%y %T", &timeinfo);

    s = buf;
}

/**
  * @описание   Получить текущее время системы в виде структуры
  * @параметры
  *     Выходные:
  *         result - системное время в виде структуры tm
 */
void get_local_time(struct tm* result)
{
    time_t seconds = time(NULL);
 
    if(!localtime_r(&seconds, result)) throw std::runtime_error(excp_msg((std::string)"localtime_r failed: " + strerror(errno)));
    // Convert to common format
    ++result->tm_mon;
    result->tm_year += 1900;

    log_msg(MSG_TRACE, "Current time: %02u.%02u.%04u %02u:%02u:%02u\n", 
        result->tm_mday, result->tm_mon, result->tm_year, 
        result->tm_hour, result->tm_min, result->tm_sec );
}

/**
  * @описание   Установка системного времени
  * @параметры
  *     Входные:
  *         hour - строка содержащая час
  *         min - строка содержащая минуты
  *         sec - строка содержащая секунды
 */
void set_time(const std::string& hour, const std::string& min, const std::string& sec)
{
    std::string cmd_ret = exec("date +\"\" --set \"" + hour + ":" + min + ":" + sec + "\" 2>&1 | tr -d '\n'");
    log_msg(MSG_VERBOSE, "set_time ret: %s\n", cmd_ret);

    if(!cmd_ret.empty()) throw std::runtime_error(excp_msg("set_time failed: " + cmd_ret));
}

/**
  * @описание   Установка системной даты
  * @параметры
  *     Входные:
  *         mday - строка содержащая число месяца
  *         month - строка содержащая месяц
  *         year - строка содержащая год
 */
void set_date(const std::string& mday, const std::string& month, const std::string& year)
{
    std::string cmd_ret = exec("date +\"\" -s \"" + year + month + mday + "\" 2>&1 | tr -d '\n'");
    log_msg(MSG_VERBOSE, "set_date ret: %s\n", cmd_ret);

    if(!cmd_ret.empty()) throw std::runtime_error(excp_msg("set_date failed: " + cmd_ret));
}

/**
  * @описание   Получение текущего абсолютного пути рабочей директории 
  * @прим      (где расположен исполняемый файл)
  * @параметры
  *     Выходные:
  *         s - строка содержащая полный путь, не включая имени исполняемого файла
 */
void get_cwd(std::string& s)
{
    int cnt = 0;
    char buf[256];

    cnt = readlink("/proc/self/exe", buf, sizeof buf);

    if(cnt <= 0) throw std::runtime_error(excp_msg((std::string)"readlink '/proc/self/exe' failed: " + strerror(errno)));

    char* sc = strrchr(buf, '/');
    if(sc) *sc = '\0'; // исключить собственное имя бинарника
    log_msg(MSG_VERBOSE, "working directory: %s\n", buf);
    s = buf;
}

/**
  * @описание   Создание новой директории если таковой еще не существует
  * @параметры
  *     Входные:
  *         path - строка содержащая полный путь создаваемой директории
  *         mode - флаги прав доступа к новой директории
 */
void make_new_dir(const std::string& path, mode_t mode)
{
    struct stat st = {0};

    if( !stat(path.c_str(), &st) ) {
        log_msg(MSG_DEBUG, "%s '%s': already exists!\n", __FUNCTION__, path);
        return; 
    }

    log_msg(MSG_VERBOSE, "creating new directory '%s'\n", path);
        
    if(mkdir(path.c_str(), mode)) std::runtime_error(excp_msg("failed to create '" + path + "': " + strerror(errno)));
}

/**
  * @описание   Изменение прав доступа к файлу 
  * @параметры
  *     Входные:
  *         path - строка содержащая полный к файлу
  *         mode - новые флаги доступа
 */
void change_mod(const std::string& path, mode_t mode)
{
    if (chmod(path.c_str(), mode)) std::runtime_error(excp_msg("failed to change mod '" + path + "': " + strerror(errno)));
}

/**
  * @описание   Получение кол-ва файлов в директории
  * @параметры
  *     Входные:
  *         dir_name - строка с именем директории
  * @возвращает Количество файлов в директории
 */
uint32_t get_files_num_in_dir(const std::string& dir_name)
{
    uint32_t file_count = 0;
    struct dirent* entry;
    std::unique_ptr<DIR, int(*)(__dirstream*)> dirp (opendir(dir_name.c_str()), closedir); 

    if(!dirp) throw std::runtime_error(excp_msg("opendir '" + dir_name + "' failed: " + strerror(errno)));

    while ((entry = readdir(dirp.get())) != nullptr) {
        if (entry->d_type == DT_REG) ++file_count;
    }

    return file_count;
}

/**
  * @описание   Получение имен файлов в директории
  * @параметры
  *     Входные:
  *         dir_name - строка с именем директории
  *     Выходные:
  *         vec - вектор строк с именами файлов
 */
void get_file_names_in_dir(const std::string& dir_name, std::vector<std::string>& vec)
{
    std::unique_ptr<DIR, int(*)(__dirstream*)> dirp (opendir(dir_name.c_str()), closedir);
    struct dirent* entry;

    if(!dirp) throw std::runtime_error(excp_msg("opendir '" + dir_name + "' failed: " + strerror(errno)));
   
    while((entry = readdir(dirp.get())) != nullptr){
        if (entry->d_type == DT_REG) vec.push_back(entry->d_name);
    }
}

/**
  * @описание   Получение размера файла
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  * @возвращает Размер файла в байтах
 */
uint64_t get_file_size (const std::string& fname)
{
    struct stat st;
    if (0 != stat(fname.c_str(), &st)) 
        throw std::runtime_error(excp_msg("File '" + fname + "' stat failed: " + strerror(errno)));

    return (uint64_t)st.st_size;
}

/**
  * @описание   Запись в бинарный файл
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  *         buf - указатель на буфер байт
  *         buf_len - размер буфера в байтах
 */
void write_bin_file(const std::string& fname, const uint8_t* buf, size_t buf_len)
{
    std::ofstream file(fname, std::ofstream::binary);

    if(!file.is_open()) throw std::runtime_error(excp_msg("File '" + fname + "' open for writing failed"));

    file.write(reinterpret_cast<const char*>(buf), buf_len);
    file.flush();
    file.close();
}

/**
  * @описание   Чтение бинарного файла
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  *     Выходные
  *         buf_len - размер прочитанного массива байт
  * @возвращает Умный указатель на прочитанный массив байт
 */
std::unique_ptr<uint8_t[]> read_bin_file(const std::string& fname, uint64_t* buf_len)
{
    std::ifstream file(fname, std::ifstream::binary);
    if(!file.is_open()) throw std::runtime_error(excp_msg("File '" + fname + "' open for reading failed"));

    uint64_t size = get_file_size(fname);
    std::unique_ptr<uint8_t[]> buf_ptr(new (std::nothrow) uint8_t[size]); 
    if(!buf_ptr) throw std::runtime_error(excp_msg("Memory allocation for '" + fname + "' buf failed"));

    file.read(reinterpret_cast<char*>(buf_ptr.get()), size);
    file.close();
    if(buf_len) *buf_len = size;
    return buf_ptr;
}

/**
  * @описание   Запись в текстовый файл потока символов
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  *         buf - указатель на массив символов для записи
  *         buf_len - размер массива символов для записи
 */
void write_text_file(const std::string& fname, const char* buf, size_t buf_len)
{
    std::ofstream file(fname);

    if(!file.is_open()) throw std::runtime_error(excp_msg("File '" + fname + "' open for writing failed"));

    file.write(buf, buf_len);
    file.flush();
    file.close();
}

/**
  * @описание   Чтение текстового файла в вектор строк
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  *     Выходные:
  *         vec - вектор прочитанных строк
 */
void read_text_file(const std::string& fname, std::vector<std::string>& vec)
{
    std::ifstream file(fname);
    if(!file.is_open()) throw std::runtime_error(excp_msg("File '" + fname + "' open open failed: " + strerror(errno)));

    std::string str;

    while(getline(file, str)) vec.push_back(str);

    file.close();
}

/**
  * @описание   Чтение текстового файла в динамическую память
  * @параметры
  *     Входные:
  *         fname - строка с именем файла
  * @возвращает Умный указатель на массив прочитанных символов в динамической памяти
 */
std::unique_ptr<char[]> read_text_file(const std::string& fname)
{
    std::ifstream file(fname);
    if(!file.is_open()) throw std::runtime_error(excp_msg("File '" + fname + "' open failed: " + strerror(errno)));

    uint64_t buf_len = get_file_size(fname);

    std::unique_ptr<char[]> buf_ptr(new (std::nothrow) char[buf_len+1]);

    if(!buf_ptr) throw std::runtime_error(excp_msg("Memory allocation for '" + fname + "' buf failed"));

    file.read(buf_ptr.get(), buf_len);
    (buf_ptr.get())[buf_len] = 0;  // конец С-строки
    file.close();

    return buf_ptr;
}

/**
  * @описание   Преобразование строки с адресом в IPv4 структуру
  * @параметры
  *     Входные:
  *         s - строка с адресом
  * @возвращает Структуру с IP адресом
 */
static ipv4_t parse_ip_str(const std::string& s)
{
    ipv4_t ip;
    std::string::size_type ppos = 0;
    std::string::size_type npos = 0;

    for(int i = 0; i < 3; ++i){
        if((npos = s.find('.', ppos)) == std::string::npos) {
            throw std::runtime_error(excp_msg("ip_v4 string has invalid format: " + s));
        }
        strcpy(ip.as_ch[i], s.substr(ppos, (npos - ppos)).c_str());
        ip.as_byte[i] = atoi(ip.as_ch[i]);
        ppos = npos + 1;
    }
    strcpy(ip.as_ch[3], s.substr(ppos, (s.length() - ppos)).c_str());
    ip.as_byte[3] = atoi(ip.as_ch[3]);

    ip.as_str = s;

    return ip;
}

/**
  * @описание   Получение имени соединения из имени интерфейса
  * @параметры
  *     Входные:
  *         iface_name - имя интерфейса в системе (eth0)
  * @возвращает Строку с именем соединения
 */
std::string get_connection_name(const std::string& iface_name)
{
    std::string name = exec("nmcli dev show " + iface_name + " | grep GENERAL.CONNECTION | cut -d':' -f 2 | cut -c 22- | tr -d '\n'");
    log_msg(MSG_VERBOSE, "connection_name: %s\n", name);
    return name;
}

/**
  * @описание   Получение текущего IP адреса системы
  * @параметры
  *     Выходные:
  *         ip - IPv4 адрес системы        
 */
void get_current_ip (ipv4_t& ip)
{
    std::string ip_str = exec("ip addr | grep -w inet | grep -v -w lo | cut -d' ' -f 6 | sed 's|/.*||' | tr -d '\n'");
    log_msg(MSG_VERBOSE, "ip_str (len = %zu): %s\n", ip_str.length(), ip_str);

    ip = parse_ip_str(ip_str);
}

/**
  * @описание   Получение текущего IP адреса системы
  * @параметры
  *     Входные: 
  *         iface_name - имя интерфейса в системе (eth0)
  *     Выходные:
  *         ip - IPv4 адрес системы
 */
void get_current_ip (const std::string& iface_name, ipv4_t& ip)
{
    std::string ip_str = exec("nmcli dev show " + iface_name + " | grep IP4.ADDRESS | sed 's|/.*||' | tr -d ' ' | cut -d':' -f 2 | tr -d '\n'");
    log_msg(MSG_VERBOSE, "ip_str (len = %zu): %s\n", ip_str.length(), ip_str);

    ip = parse_ip_str(ip_str);
}

/**
  * @описание   Получение текущего адреса сетевого шлюза
  * @параметры
  *     Входные: 
  *         iface_name - имя интерфейса в системе (eth0)
  *     Выходные:
  *         gw - IPv4 адрес шлюза
 */
void get_current_gateway(const std::string& iface_name, ipv4_t& gw)
{
    std::string gw_str = exec("nmcli dev show " + iface_name + " | grep IP4.GATEWAY | cut -d':' -f 2 | tr -d ' \n'");
    log_msg(MSG_VERBOSE, "gw_str (len = %zu): %s\n", gw_str.length(), gw_str);

    gw = parse_ip_str(gw_str);
}

/**
  * @описание   Получение статуса DHCP
  * @параметры
  *     Входные: 
  *         iface_name - имя интерфейса в системе (eth0)
  * @возвращает Текущий статус (true - включено, false - выключено)
 */
bool get_current_dhcp_state(const std::string& iface_name)
{
    std::string name = get_connection_name(iface_name);
    std::string state_str = exec("nmcli c show \"" + name + "\" | grep ipv4.method | cut -d':' -f 2 | tr -d ' \n'");
    log_msg(MSG_VERBOSE, "DHCP state_str: %s\n", state_str);

    if(state_str == "auto") return true;    // DHCP включено

    return false;
}

/**
  * @описание   Установка динамического IP адреса (DHCP)
  * @параметры
  *     Входные: 
  *         iface_name - имя интерфейса в системе (eth0)
 */
void set_dynamic_ip(const std::string& iface_name)
{
    std::string name = get_connection_name(iface_name);
    exec("nmcli con mod \"" + name + "\" ipv4.addresses \"\" ipv4.gateway \"\" ipv4.dns \"\" ipv4.dns-search \"\" ipv4.method \"auto\" ");
}

/**
  * @описание   Установка статического IP адреса
  * @параметры
  *     Входные: 
  *         iface_name - имя интерфейса в системе (eth0)
  *         ip - строка с IPv4 адресом 
  *         gw - строка с IPv4 адресом шлюза
 */
void set_static_ip(const std::string& iface_name, const std::string& ip, const std::string& gw)
{
    std::string name = get_connection_name(iface_name);
    exec("nmcli con mod \"" + name + "\" ipv4.addresses \"" + ip + "\" ipv4.gateway \"" + gw + "\" ipv4.dns \"8.8.8.8, 8.8.8.4\" ipv4.method \"manual\" ");
}

/**
  * @описание   Получение статуса точки доступа WiFi
  * @возвращает Статус работы точки доступа (true - включена, false - выключена)
 */
bool get_wifi_ap_state()
{
    std::string state = exec("systemctl status hostapd | grep Active | grep running -c");
    if (state == "1\n") return true; // Точка работает
    
    return false;
}

/**
  * @описание   Установка точки доступа WiFi
  * @параметры
  *     Входные: 
  *         on_off - переключатель (true - включить дочку доступа, false - выключить)
 */
void set_wifi_ap_state(bool on_off)
{
    std::string cmd_ret;

    if(on_off) {
        cmd_ret = exec("ifdown wlan0 && ifup wlan0 && systemctl start isc-dhcp-server && systemctl start hostapd \
&& systemctl status hostapd | grep Active | grep running -c");
        if(cmd_ret == "0\n") throw std::runtime_error(excp_msg("wifi_ap(wlan0) start failed"));
    }
    else {
        cmd_ret = exec("systemctl stop hostapd && ip link set wlan0 down");
        // TODO: check cmd_ret
    }

}

/**
  * @описание   Установка точки доступа WiFi при помощи скрипта 
  * @параметры
  *     Входные: 
  *         start_script - скрипт включения точки доступа
  *         on_off - переключатель (true - включить дочку доступа, false - выключить)
 */
void set_wifi_ap_state(const std::string& start_script, bool on_off)
{
    std::string cmd_ret;

    if(on_off) {
        cmd_ret = exec(start_script + " && systemctl status hostapd | grep Active | grep running -c");
        if(cmd_ret == "0\n") throw std::runtime_error(excp_msg("hostapd(wlan0) service start failed"));
    }
    else {
        cmd_ret = exec("systemctl stop hostapd && ip link set wlan0 down");
        // TODO: check cmd_ret
    }
}

/**
  * @описание   Получение температуры ЦПУ системы
  * @параметры
  *     Выходные: 
  *         cpu_temp - строка с температурным значением (С)
 */
void get_cpu_temp(std::string& cpu_temp)
{
    cpu_temp = exec("cat /sys/class/thermal/thermal_zone0/temp | tr -d '\n'");
    log_msg(MSG_VERBOSE, "CPU temperature: %s\n", cpu_temp);
}

/**
  * @описание   Перезагрузка системы
 */
void system_reboot()
{
    int res = std::system("reboot");
    if(res) throw std::runtime_error(excp_msg("reboot failed: " + std::to_string(WEXITSTATUS(res))));
}

/**
  * @описание   Выключение системы
 */
void system_poweroff()
{
    int res = std::system("poweroff");
    if(res) throw std::runtime_error(excp_msg("poweroff failed: " + std::to_string(WEXITSTATUS(res))));
}

} // namespace


#ifdef UNIT_TEST

#include <cinttypes>

int main(int argc, char* argv[])
{
    Log::init(MSG_TRACE, "Log.log", 1000);
    log_msg(MSG_VERBOSE, "WORKS GOOD\n");

    std::string iface_name = "enp0s3";

    try{

        log_msg(MSG_VERBOSE, "get_file_size(): %" PRIu64 "\n", utils::get_file_size("Log.log"));

        utils::make_new_dir("./new_dir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        //utils::read_bin_file("hz", nullptr);

        std::unique_ptr<char[]> buf = utils::read_text_file("../data/webtest.bat");
        log_msg(MSG_DEBUG, "buf_len: %" PRIu64 "\n%s", strlen(buf.get()), buf.get());
        
        utils::ipv4_t ip;
        utils::get_current_ip(ip);
        log_msg(MSG_DEBUG, "ip.as_str: %s, %s.%s.%s.%s, %u.%u.%u.%u\n", ip.as_str,
            ip.as_ch[0], ip.as_ch[1], ip.as_ch[2], ip.as_ch[3],
            ip.as_byte[0], ip.as_byte[1], ip.as_byte[2], ip.as_byte[3]);

        utils::get_current_ip(iface_name, ip);
        log_msg(MSG_DEBUG, "ip(iface_name).as_str: %s, %s.%s.%s.%s, %u.%u.%u.%u\n", ip.as_str,
            ip.as_ch[0], ip.as_ch[1], ip.as_ch[2], ip.as_ch[3],
            ip.as_byte[0], ip.as_byte[1], ip.as_byte[2], ip.as_byte[3]);

        utils::get_current_gateway(iface_name, ip);
        log_msg(MSG_DEBUG, "gw.as_str: %s, %s.%s.%s.%s, %u.%u.%u.%u\n", ip.as_str,
            ip.as_ch[0], ip.as_ch[1], ip.as_ch[2], ip.as_ch[3],
            ip.as_byte[0], ip.as_byte[1], ip.as_byte[2], ip.as_byte[3]);

        //utils::get_connection_name(iface_name);

        utils::get_current_dhcp_state(iface_name);

        //utils::set_time("13", "12", "11");

        //utils::set_date("4", "3", "2020");

        //Log::msg(MSG_DEBUG, "wifi_ap state: %d\n", utils::get_wifi_ap_state());

        std::string temp;
        utils::get_cpu_temp(temp);

    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }

    return 0;
}

#endif