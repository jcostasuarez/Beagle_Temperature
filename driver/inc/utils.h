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
#include <linux/delay.h> /*delay handling*/

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

/**
 * @brief 
 * 
 * @param reg 
 * @param mask 
 * @return int 
 */
int pool_register(void __iomem *reg, uint32_t mask, uint32_t value, uint32_t timeout);

/**
 * @brief Pool a boolean condition
 * 
 * @param condition boolean condition to pool
 * @param timeout timeout in miliseconds
 * @return int -EIO if the pooling took too long, or the time it took to pool/0 if the condition was met
 */
int pool_bool(volatile bool *condition, uint32_t timeout);

#endif // UTILS_H