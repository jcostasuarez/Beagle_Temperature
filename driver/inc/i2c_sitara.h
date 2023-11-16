#ifndef __I2C_COSTA_H
#define __I2C_COSTA_H

#include <linux/errno.h> /*Error handling*/
#include <linux/module.h> /*Modules handling*/
#include <linux/kernel.h> /*Kernel facilities*/
#include <linux/interrupt.h> /*Interrupt handling*/
#include <linux/pinctrl/pinctrl.h> /*Pin control*/
#include <linux/pinctrl/pinmux.h> /*Pin control*/
#include <linux/io.h> /*IO access*/
#include <linux/delay.h> /*Delay handling*/
#include <linux/mutex.h> /*Mutex handling*/

#include "types.h"

/*Funciones principales*/

/**
 * @brief Inicializa el módulo I2C
 * 
 * @return int 
 */
int i2c_sitara_init(void);

/**
 * @brief Finaliza el módulo I2C, libera los recursos tomados
 * 
 * @return int 
 */
int i2c_sitara_exit(void);

/**
 * @brief Lee un registro de un esclavo I2C
 * 
 * @param slave_address 
 * @param slave_register 
 * @param mask 
 * @param data 
 * @return int 
 */
int i2c_sitara_read(uint8_t slave_address, uint8_t slave_register, uint8_t mask, uint8_t *data);

/**
 * @brief Escribe un registro de un esclavo I2C
 * 
 * @param slave_address 
 * @param slave_register 
 * @param mask 
 * @param data 
 * @return int 
 */
int i2c_sitara_write(uint8_t slave_address, uint8_t slave_register,  uint8_t *data);

/*Funciones secundarias*/

/**
 * @brief Verifica si un esclavo I2C está conectado
 * 
 * @param slave_address 
 * @return int 
 */
int i2c_sitara_is_connected(uint8_t slave_address);

/*Definición de estructuras*/

#define I2C0_REGISTERS 0x44E0B000 /*4kb*/
#define I2C1_REGISTERS 0x4802A000 /*4kb*/
#define I2C2_REGISTERS 0x4819C000 /*4kb*/

/*OFFSETS*/
#define I2C_SITARA_REVNB_LO 0x00
#define I2C_SITARA_REVNB_HI 0x04
#define I2C_SITARA_SYSC 0x10
#define I2C_SITARA_IRQSTATUS_RAW 0x24
#define I2C_SITARA_IRQSTATUS 0x28
#define I2C_SITARA_IRQENABLE_SET 0x2C
#define I2C_SITARA_IRQENABLE_CLR 0x30
#define I2C_SITARA_WE 0x34
#define I2C_SITARA_SYSS 0x90
#define I2C_SITARA_BUF 0x94
#define I2C_SITARA_CNT 0x98
#define I2C_SITARA_DATA 0x9C
#define I2C_SITARA_CON 0xA4
#define I2C_SITARA_SA 0xAC
#define I2C_SITARA_PSC 0xB0
#define I2C_SITARA_SCLL 0xB4
#define I2C_SITARA_SCLH 0xB8

typedef struct i2c_sitara_registers
{
    /* data */
};


#endif // __I2C_COSTA_H
