/**
 * @file bmp280_cdevice.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef BMP280_CDEVICE_H
#define BMP280_CDEVICE_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#define MINOR_NUMBER 0
#define NUMBER_OF_DEVICES 1
#define DEVICE_CLASS_NAME "temp"
#define DEVICE_NAME "bmp280_sitara"

int char_device_create_bmp280(void);
void char_device_remove(void);

#endif // BMP280_CDEVICE_H