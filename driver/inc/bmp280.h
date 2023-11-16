#ifndef __BMP280_H
#define __BMP280_H

/*BIBLIOTECAS*/

#include <linnux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/errno.h>
#include <linux/cdev.h>

/*DEFINICIONES*/

/*Registros del sensor*/
#define BMP280_ADRESS_TEMP_XLSB 0XFC
#define BMP280_ADRESS_TEMP_LSB 0XFB
#define BMP280_ADRESS_TEMP_MSB 0XFA
#define BMP280_ADRESS_PRESS_XLSB 0XF9
#define BMP280_ADRESS_PRESS_LSB 0XF8
#define BMP280_ADRESS_PRESS_MSB 0XF7
#define BMP280_ADRESS_CONFIG 0XF5
#define BMP280_ADRESS_CTRL_MEAS 0XF4
#define BMP280_ADRESS_STATUS 0XF3
#define BMP280_ADRESS_RESET 0XE0
#define BMP280_ADRESS_ID 0XD0
#define BMP280_ADRESS_T1_COMP_LSB 0X88
#define BMP280_ADRESS_T1_COMP_MSB 0X88
#define BMP280_ADRESS_T2_COMP_LSB 0X8A
#define BMP280_ADRESS_T2_COMP_MSB 0X8B
#define BMP280_ADRESS_T3_COMP_LSB 0X8C
#define BMP280_ADRESS_T3_COMP_MSB 0X8D

/*TIPOS DE DATOS*/

/*! @name Calibration parameters' structure */
struct bmp280_calib_param
{
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
    int32_t t_fine;
};

/*! @name Sensor configuration structure */
struct bmp280_config
{
    uint8_t os_temp;
    uint8_t os_pres;
    uint8_t odr;
    uint8_t filter;
    uint8_t spi3w_en;
};

struct bmp280_status
{
    uint8_t measuring;
    uint8_t im_update;
};

struct bmp280_uncomp_data
{
    int32_t uncomp_temp;
    uint32_t uncomp_press;
};

/**/
/*! @name API device structure */
struct bmp280_dev
{
    uint8_t chip_id;
    uint8_t dev_id;
    uint8_t intf;
    bmp280_com_fptr_t read;
    bmp280_com_fptr_t write;
    bmp280_delay_fptr_t delay_ms;
    struct bmp280_calib_param calib_param;
    struct bmp280_config conf;
};

struct bmp280 {
    struct bmp280_cdev cdev;
    struct bmp280_dev dev;
    struct bmp280_uncomp_data uncomp_data;
    struct bmp280_status status;
};

#define BMP280_SLAVE_ADDRESS 0x76

#endif // __GY_BMP280_H