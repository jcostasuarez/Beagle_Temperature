#include "utils.h"

/**
 * @brief Set the bit 32 object
 * 
 * @param reg Dirección del registro a modificar
 * @param mask Máscara positiva, el resto de los bits se mantienen. Ejemplo: 0x1 para setear el bit 0
 * @return int 
 */
int set_bit_32(uint32_t *reg, uint32_t mask)
{
    uint32_t aux = 0;

    if(reg == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        return -ENOMEM;
    }

    aux = ioread32(reg);

    aux |= mask;

    iowrite32(aux, reg);

    return 0;
}

/**
 * @brief Clear the bit 32 object
 * 
 * @param reg Dirección del registro a modificar
 * @param mask Máscara negativa, el resto de los bits se mantienen. Ejemplo: 0x1 para limpiar el bit 0
 * @return int 
 */
int clear_bit_32(uint32_t *reg, uint32_t mask)
{
    uint32_t aux = 0;

    if(reg == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        return -ENOMEM;
    }

    aux = ioread32(reg);

    aux &= ~mask;

    iowrite32(aux, reg);

    return 0;
}

/// @brief       Realiza un pooling de un registro hasta que se cumpla una condición
/// @param reg   Dirección del registro a realizar el pooling
/// @param mask  Máscara de bits a comparar
/// @param value Valor a comparar
/// @param timeout Tiempo máximo en milisegundos de pooling
/// @return      -EIO if the pooling took too long, or the time it took to pool
int pool_register(void __iomem *reg, uint32_t mask, uint32_t value, uint32_t timeout)
{
    unsigned int counter = 0;

    while((ioread32(reg) & mask) != value)
    {
        msleep(1); //Duerme el proceso por 1 ms (No bloqueante)

        counter++;
        if(counter > timeout)
        {
            printk(KERN_ERR "pool_register: El pooling tardó demasiado.");
            return -EIO;
        }
    }

    return 0;
}

/// @brief       Realiza un pooling de un registro hasta que se cumpla una condición
/// @param reg   Dirección del registro a realizar el pooling
/// @param timeout Tiempo máximo en microsegundos
/// @return      -EIO if the pooling took too long, or the time it took to pool/ 0 if the pooling was successful
int pool_bool(volatile bool *condition, uint32_t timeout)
{
    unsigned int counter = 0;

    while(*condition == false)
    {
        msleep(1); //Duerme el proceso por 1 ms (No bloqueante)

        counter++;
        if(counter > timeout)
        {
            printk(KERN_ERR "pool_bool: El pooling tardó demasiado.");
            return -EIO;
        }
    }

    return 0;
}