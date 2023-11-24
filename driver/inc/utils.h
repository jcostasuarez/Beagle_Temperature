/**
 * @file utils.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef UTILS_H
#define UTILS_H

#include <linux/io.h> /*IO handling*/
#include <linux/types.h> /*uint32_t*/
#include <linux/kernel.h> /*printk*/
#include <linux/errno.h> /*error handling*/

/**
 * @brief Set the bit 32 object
 * 
 * @param reg 
 * @param mask 
 * @return int 
 */
int set_bit_32(uint32_t *reg, uint32_t mask);

/**
 * @brief 
 * 
 * @param reg 
 * @param mask 
 * @return int 
 */
int clear_bit_32(uint32_t *reg, uint32_t mask);

#endif // UTILS_H