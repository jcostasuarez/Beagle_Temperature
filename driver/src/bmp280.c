/**
 * @file bmp280.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief Driver para el sensor de temperatura BMP280
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/* Includes */

#include "bmp280.h"
#include "i2c_sitara.h"
#include "bmp280_cdevice.h"
#include "utils.h"

/* Static variables */

static uint32_t dig_T1;
static int32_t dig_T2;
static int32_t dig_T3;

static volatile int bmp_working = False;

/*Funciónes del módulo*/

/* Functions */

int bmp280_init(void)
{
    uint8_t aux1 = 0;
    uint8_t aux2 = 0;

    printk(KERN_INFO "bmp280_init: Inicializando el BMP280\n");

    if(bmp280_is_connected() < 0)
    {
        printk(KERN_ERR "bmp280_init: El chip no está conectado\n");
        return -1;
    }

    printk(KERN_INFO "bmp280_init: El chip está conectado\n");


    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_LSB, NOMASK, &aux2);

    dig_T1 = (aux1 << 8) | aux2;

    printk(KERN_INFO "bmp280_init: dig_T1 = %d\n", dig_T1);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, NOMASK, &aux2);

    dig_T2 = (aux1 << 8) | aux2;

    printk(KERN_INFO "bmp280_init: dig_T2 = %d\n", dig_T2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, NOMASK, &aux2);

    dig_T3 = (aux1 << 8) | aux2;

    printk(KERN_INFO "bmp280_init: dig_T3 = %d\n", dig_T3);
    
    bmp280_set_frequency(FREQ_1);
    bmp280_set_mode(BMP280_NORMAL_MODE);

    printk(KERN_INFO "bmp280_init: BMP280 configurado correctamente\n");
    return 0;

}

void bmp280_deinit(void)
{
    bmp280_set_mode(BMP280_SLEEP_MODE);

    printk(KERN_INFO "bmp280_deinit: BMP280 desconfigurado correctamente\n");
}

int bmp280_is_connected(void)
{
    uint8_t data;

    if(i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_ID,  NOMASK, &data) != BMP280_CHIP_ID)
    {
        printk(KERN_ERR "bmp280_is_connected: El chip no está conectado\n");
        return -1;
    }

    printk(KERN_INFO "bmp280_is_connected: El chip está conectado\n");
    return 0;
}

int bmp280_set_frequency(bmp280_freq_t frequency)
{
    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CONFIG, (uint8_t)frequency) < 0)
    {
        printk(KERN_ERR "bmp280_set_frequency: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

int bmp280_set_mode(bmp280_mode_t mode)
{
    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CTRL_MEAS, (uint8_t)mode) < 0)
    {
        printk(KERN_ERR "bmp280_set_mode: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

int bmp_is_running(void )
{
    if(bmp_working == False)
    {
        printk(KERN_ERR "bmp_is_running: El bmp280 no está corriendo\n");
        return -1;
    }

    printk(KERN_INFO "bmp_is_running: El bmp280 está corriendo\n");

    return bmp_working;
}

int bmp_set_running(void)
{
    if(bmp_working == True)
    {
        printk(KERN_ERR "bmp_set_running: El bmp280 ya está corriendo\n");
        return -1;
    }

    printk(KERN_INFO "bmp_set_running: El bmp280 está corriendo\n");

    bmp_working = True;

    return 0;
}

int bmp280_get_temperature(bmp280_temperature *temperature)
{
    int32_t data[3];
    int32_t raw_temp;
    int32_t var1, var2, t_fine;
    uint8_t aux1 = 0;
    uint8_t aux2 = 0;

    if(temperature == NULL)
    {
        printk(KERN_ERR "bmp280_get_temperature: El puntero es nulo\n");

        return -1;
    }

    if(i2c_sitara_is_connected(BMP280_SLAVE_ADDRESS) < 0)
    {
        printk(KERN_ERR "bmp280_get_temperature: El chip no está conectado\n");

        return -1;
    }

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_LSB, NOMASK, &aux2);

    data[0]  = (int32_t)((aux1 << 8) | aux2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, NOMASK, &aux2);

    data[1]  = (int32_t)((aux1 << 8) | aux2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, NOMASK, &aux2);

    data[2]  = (int32_t)((aux1 << 8) | aux2);

    if(data[0] < 0 || data[1] < 0 || data[2] < 0)
    {
        printk(KERN_ERR "bmp280_get_temperature: Error al leer los registros\n");

        return -1;
    }

    raw_temp = (data[0] << 16) | (data[1] << 8) | (data[2] >> 4);
    
    var1 = ((((raw_temp >> 3) - (dig_T1 << 1))) * (dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - (dig_T1)) * ((raw_temp >> 4) - (dig_T1))) >> 12) * (dig_T3)) >> 14;

    t_fine = var1 + var2;

    *temperature = (t_fine * 5 + 128) >> 8;

    return 0;
}