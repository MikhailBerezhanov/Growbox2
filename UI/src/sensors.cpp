#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "utils.h"
#include "sensors.h"

#if SENS_MUTUAL
    #include <pthread.h> 
    static pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t gpio5_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME   "[ SENS ] "
#include "dbgmsg.h"

#define I2C_ADAPTER     "/dev/i2c-0"
 
static int i2c_fd = -1;

int sensors_init(char* i2c_path)
{
    i2c_fd = open(i2c_path, O_RDWR);

    if (i2c_fd < 0) {
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to open i2c: %s\n", strerror(errno));
      return -1;
    }

    return SENS_OK;
}

void sensors_deinit(void)
{
    close(i2c_fd);
}

static void lock_i2c_mutex(void)
{
    #if SENS_MUTUAL
    pthread_mutex_lock(&i2c_mutex);
    #endif
}

static void unlock_i2c_mutex(void)
{
    #if SENS_MUTUAL
    pthread_mutex_unlock(&i2c_mutex);
    #endif
}

static void lock_gpio5_mutex(void)
{
    #if SENS_MUTUAL
    pthread_mutex_lock(&gpio5_mutex);
    #endif
}

static void unlock_gpio5_mutex(void)
{
    #if SENS_MUTUAL
    pthread_mutex_unlock(&gpio5_mutex);
    #endif
}

//https://github.com/ControlEverythingCommunity/CE_ARDUINO_LIB/blob/master/SHT30/SHT30.cpp
static uint8_t sht30_crc8(uint8_t *block, uint32_t size)
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

int sht30_read_data(float* temperature, float* humidity)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg message;
    unsigned char write_buf[2] = {0x2C, 0x06};
    unsigned char read_buf[6] = {0x00};

    /* 
     * .addr - Адрес устройства (датчика)
     * .flags - операция чтения или записи (0 - w, 1 - r)
     * .len - кол-во передаваемых/принимаемых сообщений
     * .buf - буфер на чтение или запись
     */
    message.addr = 0x44;
    message.flags = 0;
    message.len = 2;
    message.buf = write_buf;

    data.msgs = &message;
    data.nmsgs = 1;

    lock_i2c_mutex();

    if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "i2c write failed: %s\n", strerror(errno));
      unlock_i2c_mutex();
      return -1; 
    }

    message.addr = 0x44;
    message.flags = 1;
    message.len = sizeof(read_buf);
    message.buf = read_buf;

    if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "i2c read failed: %s\n", strerror(errno));
      unlock_i2c_mutex();
      return -1;
    }
    usleep(100000); // Hold I2C bus
    unlock_i2c_mutex();

    dbgmsg_v(MSG_VERBOSE, "SHT30 Responce: ");
    for (uint i = 0; i < sizeof(read_buf); ++i){
        dbgmsg_v(MSG_VERBOSE, "0x%02X ", read_buf[i]);
    }
    dbgmsg_v(MSG_VERBOSE, "\n");

    // Convert the data
    uint8_t my_crc = sht30_crc8(read_buf, 2);

    if (read_buf[2] == my_crc){
        float cTemp = ((((read_buf[0] * 256.0) + read_buf[1]) * 175) / 65535.0) - 45;
        float fTemp = (cTemp * 1.8) + 32;
        dbgmsg_v(MSG_VERBOSE, "Temperature in Celsius: %f C\n", cTemp);
        dbgmsg_v(MSG_VERBOSE, "Temperature in Fahrenheit: %f F\n", fTemp);
        if(temperature) *temperature = cTemp;
    }
    else {
      dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Temperature CRC error. Inc: 0x%02X, My: 0x%02X\n", read_buf[2], my_crc);
      return -2;
    }

    my_crc = sht30_crc8(&read_buf[3], 2);

    if (read_buf[5] == my_crc){
        float hum = ((((read_buf[3] * 256.0) + read_buf[4]) * 100) / 65535.0);
        dbgmsg_v(MSG_VERBOSE, "Relative Humidity: %f RH\n", hum);
        if(humidity) *humidity = hum;
    }
    else {
      dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Humidity CRC error. Inc: 0x%02X, My: 0x%02X\n", read_buf[5], my_crc);
      return -3;
    }

    return SENS_OK;
}


int set_coolers_power(cooler_t name, uint8_t power)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg message;
    unsigned char write_buf[2];

    if(name == input) write_buf[0] = 0x10;
    else write_buf[0] = 0x20;
    write_buf[1] = (power > 100) ? 100 : power;

    dbgmsg_v(MSG_VERBOSE, "Setting Cooler%u power: %u\n", name, write_buf[1]);

    message.addr = 0x43;
    message.flags = 0;
    message.len = 2;
    message.buf = write_buf;

    data.msgs = &message;
    data.nmsgs = 1;

    lock_i2c_mutex();
    if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "i2c write failed: %s\n", strerror(errno));
      unlock_i2c_mutex();
      return -1; 
    }
    usleep(100000); // Hold I2C bus
    unlock_i2c_mutex();

    return SENS_OK;
}


int get_coolers_power(cooler_t name, uint8_t* power)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg messages[2];
    unsigned char write_buf[1];   // register address
    unsigned char read_buf[1];    // output data

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

    lock_i2c_mutex();
    if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "i2c read failed: %s\n", strerror(errno));
      unlock_i2c_mutex();
      return -1; 
    }
    usleep(100000); // Hold I2C bus
    unlock_i2c_mutex();

    dbgmsg_v(MSG_VERBOSE, "Cooler%u power: %u\n", name, read_buf[0]);
    if(power) *power = read_buf[0];

    return SENS_OK;
}


int get_water_sensor(char* ws_path, uint* value)
{
    int res = -1;
    char cmd[256];
    char *val_str = NULL;
    uint val = 0;
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "gpio read 16 > %s", ws_path);

    lock_gpio5_mutex();
    res = system(cmd);

    if(res){
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "read water_sensor gpio failed (%d)\n", WEXITSTATUS(res));
      unlock_gpio5_mutex();
      return WEXITSTATUS(res);
    }
    val_str = read_text_file(ws_path, posix_filesize(ws_path));
    unlock_gpio5_mutex();

    if (!val_str){
      errmsg_v(MSG_DEBUG | MSG_TO_FILE, "read water_sensor file failed\n");
      return -1;
    }

    sscanf(val_str, "%u", &val);
    dbgmsg_v(MSG_VERBOSE, "Water sensor value: %u\n", val);

    free(val_str);
    if(value) *value = val;

    return SENS_OK;
}

