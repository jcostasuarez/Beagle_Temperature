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