/**
 * @file bmp280.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief Definición de funciones y estructuras para el manejo del sensor BMP280
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __BMP280_H
#define __BMP280_H

/*BIBLIOTECAS*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/of.h>


#include "types.h"

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

#define BMP280_CHIP_ID 0x58
#define BMP280_RESET_VALUE 0xB6

#define NOMASK 0xFF

/* Types definitions */

#define BMP280_SLAVE_ADDRESS 0x76

/*Typedef*/

/*Enums*/

enum bool
{
    False = 0,
    True = 1
};

typedef enum bmp280_mode
{
    BMP280_SLEEP_MODE = 0x00,
    BMP280_FORCED_MODE = 0x01,
    BMP280_NORMAL_MODE = 0x03,
    BMP280_SOFT_RESET_CODE = 0xB6
} bmp280_mode_t;

typedef enum bmp280_standby_duration
{
    BMP280_STANDBY_TIME_1_MS = 0x00,
    BMP280_STANDBY_TIME_63_MS = 0x01,
    BMP280_STANDBY_TIME_125_MS = 0x02,
    BMP280_STANDBY_TIME_250_MS = 0x03,
    BMP280_STANDBY_TIME_500_MS = 0x04,
    BMP280_STANDBY_TIME_1000_MS = 0x05,
    BMP280_STANDBY_TIME_2000_MS = 0x06,
    BMP280_STANDBY_TIME_4000_MS = 0x07
} bmp280_standby_duration_t;

typedef enum bmp280_filter_coefficient
{
    BMP280_FILTER_COEFF_OFF = 0x00,
    BMP280_FILTER_COEFF_2 = 0x01,
    BMP280_FILTER_COEFF_4 = 0x02,
    BMP280_FILTER_COEFF_8 = 0x03,
    BMP280_FILTER_COEFF_16 = 0x04
} bmp280_filter_coefficient_t;

typedef enum bmp280_oversampling
{
    BMP280_NO_OVERSAMPLING = 0x00,
    BMP280_OVERSAMPLING_1X = 0x01,
    BMP280_OVERSAMPLING_2X = 0x02,
    BMP280_OVERSAMPLING_4X = 0x03,
    BMP280_OVERSAMPLING_8X = 0x04,
    BMP280_OVERSAMPLING_16X = 0x05
} bmp280_oversampling_t;

typedef enum bmp280_sensor_mode
{
    BMP280_ULTRALOWPOWER_MODE = 0x00,
    BMP280_LOWPOWER_MODE = 0x01,
    BMP280_STANDARDRESOLUTION_MODE = 0x02,
    BMP280_HIGHRESOLUTION_MODE = 0x03,
    BMP280_ULTRAHIGHRESOLUTION_MODE = 0x04
} bmp280_sensor_mode_t;

typedef uint32_t bmp280_temperature;
typedef uint32_t bmp280_pressure;
typedef uint32_t bmp280_humidity;

/*Prototipos de funciones*/

/**
 * @brief 
 * 
 * @param temperature 
 * @return int 
 */
int bmp280_get_temperature(int *temperature);

/**
 * @brief Función para inicializar el BMP280
 *          - Realiza un soft reset
 *          - 
 * @return int 
 */
int bmp280_init(void);

/**
 * @brief Función para poner al BMP280 en modo sleep
 */
void bmp280_deinit(void);

/**
 * @brief Funcion para corroborar que el BMP280 esté conectado
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_is_connected(void);

/**
 * @brief Configura el registro ctrl_meas del BMP280
 * 
 * @param mode Modo de funcionamiento
 * @param orst_t Oversampling de temperatura. 
 * @param orst_p Oversampling de presión
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_ctrl_meas(bmp280_mode_t mode, bmp280_oversampling_t orst_t, bmp280_oversampling_t orst_p);

/**
 * @brief Configura el registro config del BMP280
 * 
 * @param t_sb Tiempo de espera entre mediciones
 * @param filter Coeficiente de filtrado IRR
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_config( bmp280_standby_duration_t t_sb, bmp280_filter_coefficient_t filter);

/**
 * @brief Realiza un soft reset del BMP280
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_soft_reset(void);

#endif // __GY_BMP280_H