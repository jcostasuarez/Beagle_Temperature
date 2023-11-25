/**
 * @file i2c_sitara.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief Definición de funciones y estructuras para el manejo del módulo I2C de la BeagleBone Black
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __I2C_COSTA_H
#define __I2C_COSTA_H

#include <linux/errno.h> /*Error handling*/
#include <linux/module.h> /*Modules handling*/
#include <linux/kernel.h> /*Kernel facilities*/
#include <linux/irqreturn.h> /*IRQ handling*/
#include <linux/interrupt.h> /*Interrupt handling*/
#include <linux/pinctrl/pinctrl.h> /*Pin control*/
#include <linux/pinctrl/pinmux.h> /*Pin control*/
#include <linux/delay.h> /*Delay handling*/
#include <linux/mutex.h> /*Mutex handling*/
#include <linux/io.h> /*IO handling*/
#include <linux/platform_device.h> /*Platform devices handling*/
#include <linux/init.h> /*Init handling*/

#include "types.h"

/*
    Device tree overlay:
    compatible = "ti,sysc-omap2\0ti,sysc";
    reg = <0x9c000 0x08 0x9c010 0x08 0x9c090 0x08>;
    reg-names = "rev\0sysc\0syss";
    ti,sysc-mask = <0x307>;
    ti,sysc-sidle = <0x00 0x01 0x02 0x03>;
    ti,syss-mask = <0x01>;
    clocks = <0x2f 0x0c 0x00>;
    clock-names = "fck";
    #address-cells = <0x01>;
    #size-cells = <0x01>;
    ranges = <0x00 0x9c000 0x1000>;
*/

/* Definición de registros */

#define CTRL_MODULE_BASE         0x44E10000
#define CTRL_MODULE_SIZE     0x00002000
#define CONF_UART1_CSTN         0x00000978
#define CONF_UART1_RSTN         0x0000097C
#define PIN_I2C_CFG             0x00000023


#define CM_PER_BASE 0x44E00000
#define CM_PER_SIZE 0x400

#define CM_PER_I2C2_CLKCTRL_OFFSET 0x44

#define CM_WKP_BASE 0x44E00400
#define CM_WKP_SIZE 0x100

#define CM_WKP_CLKMODE_DPLL_PER_OFFSET 0x8C
#define CM_WKP_IDLEST_DPLL_PER_OFFSET 0x70
#define CM_WKP_DIV_M2_DPLL_PER_OFFSET 0xAC
#define CM_WKP_CLKSEL_DPLL_PER_OFFSET 0x9C

#define I2C_SITARA_I2C2_BASE 0x4819C000
#define I2C_SITARA_I2C2_SIZE 0x1000

#define I2C_SITARA_CON_EN 0x1<<15
#define I2C_SITARA_CON_MST 0x1<<10
#define I2C_SITARA_CON_STT 0x1<<0
#define I2C_SITARA_CON_STP 0x1<<1
#define I2C_SITARA_CON_TRX 0x1<<9
#define I2C_SITARA_CON_XSA 0x1<<8


#define I2C_SITARA_AL   0x1<<0
#define I2C_SITARA_NACK 0x1<<1
#define I2C_SITARA_ARDY 0x1<<2
#define I2C_SITARA_RRDY 0x1<<3
#define I2C_SITARA_XRDY 0x1<<4
#define I2C_SITARA_GC   0x1<<5
#define I2C_SITARA_STC  0x1<<6
#define I2C_SITARA_AERR 0x1<<7
#define I2C_SITARA_BF   0x1<<8
#define I2C_SITARA_AAS  0x1<<9
#define I2C_SITARA_XUDF 0x1<<10
#define I2C_SITARA_ROVR 0x1<<11
#define I2C_SITARA_BB   0x1<<12
#define I2C_SITARA_RDR  0x1<<13
#define I2C_SITARA_XDR  0x1<<14

#define I2C_SITARA_BUF_RXFIFO_CLR 0x1<<14
#define I2C_SITARA_BUF_TXFIFO_CLR 0x1<<6

#define I2C_SITARA_SYSC_SRST 0x2

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

/*Funciones principales*/

/**
 * @brief Inicializa el módulo I2C
 * 
 * @return int 
 */
int i2c_sitara_init(struct platform_device *pdev);

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
int i2c_sitara_write(uint8_t slave_address, uint8_t slave_register,  uint8_t data);

/*Funciones secundarias*/

/**
 * @brief Verifica si un esclavo I2C está conectado
 * 
 * @param slave_address 
 * @return int 
 */
int i2c_sitara_is_connected(uint8_t slave_address);


/*Definición de estructuras*/

typedef struct i2c_sitara_registers
{
    uint32_t revnb_lo;
    uint32_t revnb_hi;
    uint32_t sysc;
    uint32_t irqstatus_raw;
    uint32_t irqstatus;
    uint32_t irqenable_set;
    uint32_t irqenable_clr;
    uint32_t we;
    uint32_t syss;
    uint32_t buf;
    uint32_t cnt;
    uint32_t data;
    uint32_t con;
    uint32_t sa;
    uint32_t psc;
    uint32_t scll;
    uint32_t sclh;
} i2c_sitara_registers_t;


#endif // __I2C_COSTA_H
