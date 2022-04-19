#include <cstdint>
#include <cerrno>
#include <string>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "utils.h"
#include "sensors.h"

#define LOG_MODULE_NAME     "[ SENS ] "
#include "loger.h"

// Инициализация статических членов класса
int Sensors::i2c_fd = -1;
std::mutex Sensors::i2c_mutex;
std::mutex Sensors::water_sensor_mutex;

void Sensors::init(const char* i2c_dev)
{
    i2c_fd = open(i2c_dev, O_RDWR);

    if (i2c_fd < 0) {
      throw std::runtime_error(excp_msg("Unabke to open '" + (std::string)i2c_dev + "': " + strerror(errno)));
    }
}

void Sensors::deinit() 
{ 
    if(i2c_fd < 0) return;

    close(i2c_fd); 
    i2c_fd = -1;
}


//https://github.com/ControlEverythingCommunity/CE_ARDUINO_LIB/blob/master/SHT30/SHT30.cpp
static uint8_t SHT30_CRC8(uint8_t* block, uint32_t size)
{
    const uint8_t POLYNOMIAL = 0x31;
    uint8_t CRC = 0xFF;
    
    for ( int j = size; j; --j )
    {
        CRC ^= *block++;
        for ( int i = 8; i; --i )
        {
            CRC = ( CRC & 0x80 )
            ? (CRC << 1) ^ POLYNOMIAL
            : (CRC << 1);
        }
    }
    return CRC;
}

void Sensors::SHT30_read(float& temperature, float& humidity)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg message;
    uint8_t write_buf[2] = {0x2C, 0x06};
    uint8_t read_buf[6] = {0x00};

    /* 
     * .addr - Адрес устройства (датчика)
     * .flags - операция чтения или записи (0 - w, 1 - r)
     * .len - кол-во передаваемых/принимаемых сообщений
     * .buf - буфер на чтение или запись
     */
    message.addr = SHT30_ADDR;
    message.flags = 0;
    message.len = 2;
    message.buf = write_buf;

    data.msgs = &message;
    data.nmsgs = 1;

    // Синхронизация доступа к шине
    {
        std::lock_guard<std::mutex> lock(i2c_mutex);
    
        if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
          throw std::runtime_error(excp_msg((std::string)"i2c write failed: " + strerror(errno))); 
        }
    
        message.addr = SHT30_ADDR;
        message.flags = 1;
        message.len = sizeof(read_buf);
        message.buf = read_buf;
    
        if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
          throw std::runtime_error(excp_msg((std::string)"i2c read failed: " + strerror(errno))); 
        }
        usleep(100000); // FIXIT: Hold I2C bus
    }

    Log::hex_dump(MSG_TRACE, read_buf, sizeof(read_buf), "SHT30 Response: ");

    uint8_t my_crc = SHT30_CRC8(read_buf, 2);
    if (read_buf[2] != my_crc){
        throw std::runtime_error(excp_msg("Temperature CRC error. Inc: " + std::to_string(read_buf[2]) + "My: " + std::to_string(my_crc)));
    }

    // Преобразование прочитанных данных в показания температуры
    temperature = ((((read_buf[0] * 256.0) + read_buf[1]) * 175) / 65535.0) - 45;
    float f_temperature = (temperature * 1.8) + 32;
    log_msg(MSG_VERBOSE, "Temperature in Celsius: %f C\n", temperature);
    log_msg(MSG_VERBOSE, "Temperature in Fahrenheit: %f F\n", f_temperature);


    my_crc = SHT30_CRC8(&read_buf[3], 2);

    if (read_buf[5] != my_crc){
        throw std::runtime_error(excp_msg("Humidity CRC error. Inc: " + std::to_string(read_buf[5]) + "My: " + std::to_string(my_crc)));
    }

    // Преобразование прочитанных данных в показания температуры
    humidity = ((((read_buf[3] * 256.0) + read_buf[4]) * 100) / 65535.0);
    log_msg(MSG_VERBOSE, "Relative Humidity: %f RH\n", humidity);
}


void Sensors::set_coolers_power(cooler_type name, const uint8_t power)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg message;
    uint8_t write_buf[2];

    if(name == input) write_buf[0] = 0x10;
    else write_buf[0] = 0x20;
    write_buf[1] = (power > 100) ? 100 : power;

    log_msg(MSG_VERBOSE, "Setting Cooler%u power: %u\n", name, write_buf[1]);

    message.addr = SENSORS_BOARD_ADDR;
    message.flags = 0;
    message.len = sizeof(write_buf);
    message.buf = write_buf;

    data.msgs = &message;
    data.nmsgs = 1;

    // Синхронизация доступа к шине
    std::lock_guard<std::mutex> lock(i2c_mutex);

    if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
      throw std::runtime_error(excp_msg((std::string)"i2c write failed: " + strerror(errno))); 
    }
    usleep(100000); // Hold I2C bus
}


void Sensors::get_coolers_power(cooler_type name, uint8_t& power)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg messages[2];
    uint8_t write_buf[1];   // register address
    uint8_t read_buf[1];    // output data

    if(name == input) write_buf[0] = 0x11;
    else write_buf[0] = 0x21;

    messages[0].addr = 0x43;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = write_buf;

    messages[1].addr = 0x43;
    messages[1].flags = 1; //I2C_M_RD | I2C_M_NOSTART
    messages[1].len = 1;
    messages[1].buf = read_buf;

    data.msgs = messages;
    data.nmsgs = 2;

    // Синхронизация доступа к шине
    {
        std::lock_guard<std::mutex> lock(i2c_mutex);

        if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
          throw std::runtime_error(excp_msg((std::string)"i2c read failed: " + strerror(errno))); 
        }
        usleep(100000); // Hold I2C bus
    }

    log_msg(MSG_VERBOSE, "Cooler%u power: %u\n", name, read_buf[0]);
    power = read_buf[0];
}


bool Sensors::get_water_sensor()
{
    std::string value_str;
    std::string cmd{"gpio read 16"};

    // Синхронизация доступа к GPIO
    {
        std::lock_guard<std::mutex> lock(water_sensor_mutex);

        value_str = utils::exec(cmd);
    }

    uint value = 0;
    sscanf(value_str.c_str(), "%u", &value);
    bool res = value ? true : false;

    log_msg(MSG_TRACE, "Water sensor value: %s\n", res);

    return res;
}




#ifdef UNIT_TEST

int main(int argc, char* argv[])
{
    Log::init(MSG_TRACE, "Log.log", 1000);

    std::string test = "sensors test"; 
    log_msg(MSG_DEBUG, "'%s' starts\n", test);

    try {
        Sensors sens;

        sens.get_water_sensor();
    }
    catch(const std::runtime_error& e){
        log_msg(MSG_DEBUG | MSG_TO_FILE, "%s\n", e.what());
    }
}

#endif