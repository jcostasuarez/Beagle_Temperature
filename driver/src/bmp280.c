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

#define READ_DELAY 5
/* Static variables */

static unsigned short dig_T1=0;
static short dig_T2=0;
static short dig_T3=0;

/*Funciónes del módulo*/

/* Functions */

int bmp280_init(void)
{
    uint8_t aux1 = 0;
    uint8_t aux2 = 0;

    printk(KERN_INFO "bmp280_init: Inicializando el BMP280\n");

<<<<<<< HEAD
    if(bmp280_is_connected() < 0)
=======
    if(bmp280_is_connected()!= 0)
>>>>>>> tmp
    {
        printk(KERN_ERR "bmp280_init: El BMP280 no esta conectado\n");
        return -1;
    }

<<<<<<< HEAD
    printk(KERN_INFO "bmp280_init: El chip está conectado\n");

=======
    if(bmp280_soft_reset() != 0)
    {
        printk(KERN_ERR "bmp280_init: Error al resetear el BMP280\n");
        return -1;
    }
    
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_MSB, &aux1);
>>>>>>> tmp

    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_LSB, &aux2);

    dig_T1 = ((unsigned short)aux1 << 8) | (unsigned short)aux2;

<<<<<<< HEAD
    printk(KERN_INFO "bmp280_init: dig_T1 = %d\n", dig_T1);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, NOMASK, &aux2);
=======
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, &aux1);
>>>>>>> tmp

    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, &aux2);

<<<<<<< HEAD
    printk(KERN_INFO "bmp280_init: dig_T2 = %d\n", dig_T2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, NOMASK, &aux2);
=======
    dig_T2 = ((short)aux1 << 8) | (unsigned short)aux2;
>>>>>>> tmp

    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, &aux1);

<<<<<<< HEAD
    printk(KERN_INFO "bmp280_init: dig_T3 = %d\n", dig_T3);
    
    bmp280_set_frequency(FREQ_1);
    bmp280_set_mode(BMP280_NORMAL_MODE);
=======
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, &aux2);

    dig_T3 = ((short)aux1 << 8) | (unsigned short)aux2;

    bmp280_ctrl_meas(BMP280_NORMAL_MODE, BMP280_OVERSAMPLING_1X, BMP280_OVERSAMPLING_4X);
    bmp280_config(BMP280_STANDBY_TIME_1_MS, BMP280_FILTER_COEFF_16);

    printk(KERN_INFO "bmp280_init: dig_T1 = %u\n", dig_T1);
    printk(KERN_INFO "bmp280_init: dig_T2 = %i\n", dig_T2);
    printk(KERN_INFO "bmp280_init: dig_T3 = %i\n", dig_T3);
>>>>>>> tmp

    printk(KERN_INFO "bmp280_init: BMP280 configurado correctamente\n");
    return 0;

}

void bmp280_deinit(void)
{
    bmp280_ctrl_meas(BMP280_SLEEP_MODE, BMP280_OVERSAMPLING_1X, BMP280_OVERSAMPLING_1X);
    
    dig_T1 = 0;
    dig_T2 = 0;
    dig_T3 = 0;

    printk(KERN_INFO "bmp280_deinit: BMP280 desconfigurado correctamente\n");
}

int bmp280_is_connected(void)
{
    uint8_t data;

    printk(KERN_INFO "bmp280_is_connected: Verificando si el chip estA conectado\n");

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_ID, &data);

    if(data != BMP280_CHIP_ID)
    {
        printk(KERN_ERR "bmp280_is_connected: El chip no estA conectado\n");
        return -1;
    }

    printk(KERN_INFO "bmp280_is_connected: El chip estA conectado\n");
    return 0;
}

/**
 * @brief Configura el registro ctrl_meas del BMP280
 * 
 * @param mode Modo de funcionamiento
 * @param orst_t Oversampling de temperatura. 
 * @param orst_p Oversampling de presión
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_ctrl_meas(bmp280_mode_t mode, bmp280_oversampling_t orst_t, bmp280_oversampling_t orst_p)
{
    uint8_t aux = 0;

    aux = (uint8_t)mode | (uint8_t)orst_t << 5 | (uint8_t)orst_p << 2;

    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CTRL_MEAS, aux) !=0)
    {
        printk(KERN_ERR "bmp280_set_mode: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Configura el registro config del BMP280
 * 
 * @param t_sb Tiempo de espera entre mediciones
 * @param filter Coeficiente de filtrado IRR
 * @return int 1 si hubo un error, 0 si no
 */
int bmp280_config( bmp280_standby_duration_t t_sb, bmp280_filter_coefficient_t filter)
{
    uint8_t aux = 0;

    aux = (uint8_t)t_sb << 5 | (uint8_t)filter << 2;

    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CONFIG, aux) !=0)
    {
        printk(KERN_ERR "bmp280_set_mode: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

int bmp280_get_temperature(int *temperature)
{
    int8_t temp_msb;
    uint8_t temp_lsb;
    uint8_t temp_xlsb;
    int32_t raw_temp;
    int32_t var1, var2;
    int32_t t_fine;

    if(temperature == NULL)
    {
        printk(KERN_ERR "bmp280_get_temperature: El puntero es nulo\n");

        return -1;
    }

    // Read temp_msb, temp_lsb and temp_xlsb
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_TEMP_MSB, &temp_msb);
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_TEMP_LSB, &temp_lsb);
    msleep(READ_DELAY);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_TEMP_XLSB, &temp_xlsb);

    // Convert the data to 20-bits

    raw_temp = (((uint32_t)temp_msb << 12) | ((uint32_t)temp_lsb << 4) | ((uint32_t)temp_xlsb >> 4));


    var1 = ((((raw_temp >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)dig_T1)) * ((raw_temp >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    
    t_fine = var1 + var2;

    *temperature = ((t_fine * 5 + 128) >> 8);

    //print all the values
    printk(KERN_INFO "bmp280_get_temperature: raw_temp = %d\n", raw_temp);
    printk(KERN_INFO "bmp280_get_temperature: var1 = %d\n", var1);
    printk(KERN_INFO "bmp280_get_temperature: var2 = %d\n", var2);
    printk(KERN_INFO "bmp280_get_temperature: t_fine = %d\n", t_fine);
    printk(KERN_INFO "bmp280_get_temperature: temperature = %d\n", *temperature);
    printk(KERN_INFO "bmp280_get_temperature: temp_msb = %d\n", temp_msb);
    printk(KERN_INFO "bmp280_get_temperature: temp_lsb = %d\n", temp_lsb);
    printk(KERN_INFO "bmp280_get_temperature: temp_xlsb = %d\n", temp_xlsb);

    return 0;
}

int bmp280_soft_reset(void)
{
    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_RESET, BMP280_RESET_VALUE) != 0)
    {
        printk(KERN_ERR "bmp280_soft_reset: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}