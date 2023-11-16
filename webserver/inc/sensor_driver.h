#ifndef __GY_BMP280_H
#define __GY_BMP280_H

#include <stdio.h>
#include "../../driver/inc/gy_bmp280.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "sensor_driver.h"

#include "sensor_driver.h"

#define TEMP_OVERSAMPLING_REG 0xf4
#define TEMP_OVERSAMPLING_SETTING 0x01
#define TEMP_DATA_REG 0xfa

#define I2C_BUS "/dev/i2c-1"
#define BMP280_ADDR 0x76

/// @brief 
/// @param  
/// @return 
size_t init_sensor(void);


/// @brief 
/// @param  
/// @return 
size_t get_temp(float * temp);

#endif // __GY_BMP280_H

