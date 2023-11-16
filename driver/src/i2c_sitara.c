/*Funciones principales*/
#include "i2c_sitara.h"

/* Funciones secundarias, privadas */

static int i2c_sitara_config_peripheral_clock(void);
static int i2c_sitara_config_pinmux(void);
static int i2c_sitara_config_regs(void);
static int i2c_sitara_config_interrupts(void);
static int i2c_sitara_free_interrupts(void);
static int i2c_sitara_config_clock(void);
static int i2c_sitara_start(unsigned int dcount);
static int i2c_reset_fifos(void);
static int pool_register(void __iomem *reg, uint32_t mask, uint32_t value, uint32_t timeout);
static irqreturn_t  i2c_sitara_irq_handler (int irq, void *dev id, struct pt regs*regs);

/* Variables globales, privadas */

/// @brief Variable que tiene mapeado los registros de i2c2
static void __iomem *i2c2_registers = NULL;

/* Definición de registros */
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

#define I2C_SITARA_CON_EN 0x8000
#define I2C_SITARA_CON_MST 0x400
#define I2C_SITARA_CON_STT 0x1
#define I2C_SITARA_CON_STP 0x2
#define I2C_SITARA_CON_TRX 0x200
#define I2C_SITARA_CON_XSA 0x100

#define I2C_SITARA_IRQENABLE_SET_NACK   0x2
#define I2C_SITARA_IRQENABLE_SET_ARDY   0x4
#define I2C_SITARA_IRQENABLE_SET_RRDY   0x8
#define I2C_SITARA_IRQENABLE_SET_XRDY   0x10
#define I2C_SITARA_IRQENABLE_SET_BF     0x100
#define I2C_SITARA_IRQENABLE_SET_BB     0x1000

#define I2C_SITARA_NACK_MASK 0x2
#define I2C_SITARA_ARDY_MASK 0x4
#define I2C_SITARA_RRDY_MASK 0x8
#define I2C_SITARA_XRDY_MASK 0x10
#define I2C_SITARA_BF_MASK   0x100
#define I2C_SITARA_BB_MASK   0x1000

#define I2C_SITARA_BUF_RXFIFO_CLR 0x4000
#define I2C_SITARA_BUF_TXFIFO_CLR 0x40

#define IRQ_I2C2 30

/*volatil*/

static volatile bool data_rdy = false;

/******** Funciones públicas ********/

/**
 * @brief 
 * 
 * @return int 
 */
int i2c_sitara_init()
{
    int ret_val = 0;

    /*Configuro los registros del i2c*/
    i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE);

    if(i2c2_registers == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        iounmap(i2c2_registers);
        return -ENOMEM;
    }

    /*Configuro el clock de periféricos*/
    if(ret_val = i2c_sitara_config_peripheral_clock(void) != 0)
    {
        printk( KERN_ERR "Error al configurar el clock de periféricos\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    /*Configuro los pines del I2C2*/
    if(ret_val = i2c_sitara_config_pinmux(void) != 0)
    {
        printk( KERN_ERR "Error al configurar los pines del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    /*Configuro el clock del I2C2*/

    if(ret_val = i2c_sitara_config_clock(void))
    {
        printk( KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    /*Configuro las interrupciones del I2C2*/

    if(ret_val = i2c_sitara_config_interrupts(void) != 0)
    {
        printk( KERN_ERR "Error al configurar las interrupciones del I2C2\n")
        iounmap(i2c2_registers);
        return ret_val;
    }

    /*Configuro los registros del I2C2 y habilito el bus*/

    if(ret_val = i2c_sitara_config_regs(i2c2_registers) != 0)
    {
        printk( KERN_ERR "Error al configurar los registros del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "Inicialización de i2c2 exitosa\n");
    return ret_val;
}

/**
 * @brief 
 * 
 * @return int 
 */
int i2c_sitara_exit(void)
{
    int ret_val = 0;

    if(ret_val = free_irq(IRQ_I2C2, NULL) != 0)
    {
        printk(KERN_ERR "Error al liberar la interrupción del I2C2\n");
        return ret_val;
    }

    if(i2c2_registers!=NULL)
    {
        if(ret_val = iounmap(i2c2_registers))
        {
            printk(KERN_ERR "Error al liberar la memoria de i2c2_registers\n");
            return ret_val;
        }

    }

    printk(KERN_INFO "Liberación de interrupciones i2c2 exitosa\n");
    return ret_val;
}

/**
 * @brief 
 * 
 * @param slave_address 
 * @param slave_register 
 * @param mask 
 * @param data 
 * @return int 
 */
int i2c_sitara_read(uint8_t slave_address, uint8_t slave_register, uint8_t mask, uint8_t *data)
{
    int ret_val = 0;
    uint32_t aux = 0;
    unsigned int timeout = 0;

    if(i2c2_registers==NULL)
    {
        if(i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE) == NULL)
        {
            printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
            return -ENOMEM;
        }
    }

    /* Pool del bit BB del registro I2C_IRQSTATUS_RAW*/

    if(ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB_MASK, I2C_SITARA_IRQENABLE_SET_BB, 1000) != 0)
    {
        printk(KERN_ERR "Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }

    /*Reset FIFOs*/

    if(ret_val=i2c_reset_fifos())
    {
        printk(KERN_ERR "Error al resetear los FIFOs\n");
        return ret_val;
    }

    /*Set slave address*/

    aux = (slave_address << 1) | 0x1;
    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_SA) != 0)
    {
        printk(KERN_ERR "Error al escribir la dirección del esclavo\n");
        return ret_val;
    }

    /*Set data*/

    ret_val = iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA) & mask;

    if(ret_val)
    {
        printk(KERN_ERR "Error al escribir el registro del esclavo\n");
        return ret_val;
    }

    /*Start*/

    if(ret_val = i2c_sitara_start(1) != 0)
    {
        printk(KERN_ERR "Error al iniciar el bus\n");
        return ret_val;
    }

    /* Pool para recuperar el dato */

    timeout = 1000;

    while( timeout > 0)
    {
        if(data_rdy == true)
        {
            data_rdy = false;
            break;
        }

        msleep(1);
        timeout--;
    }
    
    printk(KERN_INFO "Timeout: %d\n", timeout);

    if(timeout == 0)
    {
        printk(KERN_ERR "No se pudo leer el registro\n");
        return -EIO;
    }
    /*Read Data*/

    *data = iowread32(i2c2_registers+I2C_SITARA_DATA) & mask;

    return ret_val;
}

/**
 * @brief 
 * 
 * @param slave_address 
 * @param slave_register 
 * @param mask 
 * @param data 
 * @return int 
 */
int i2c_sitara_write(uint8_t slave_address, uint8_t slave_register,  uint8_t *data)
{
    int ret_val = 0;
    uint32_t aux = 0;

    if(i2c2_registers==NULL)
    {
        if(i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE) == NULL)
        {
            printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
            return -ENOMEM;
        }
    }

    /* Pool del bit BB del registro I2C_IRQSTATUS_RAW*/

    if(ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB_MASK, I2C_SITARA_IRQENABLE_SET_BB, 1000) != 0)
    {
        printk(KERN_ERR "Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }

    /*Reset FIFOs*/

    if(ret_val=i2c_reset_fifos())
    {
        printk(KERN_ERR "Error al resetear los FIFOs\n");
        return ret_val;
    }

    /*Set slave address*/

    aux = (slave_address << 1) | 0x0;

    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_SA) != 0)
    {
        printk(KERN_ERR "Error al escribir la dirección del esclavo\n");
        return ret_val;
    }

    /*Write Data*/

    if(ret_val = iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA))
    {
        printk(KERN_ERR "Error al escribir el registro del esclavo\n");
        return ret_val;
    }

    if(ret_val = iowrite32(*data, i2c2_registers+I2C_SITARA_DATA))
    {
        printk(KERN_ERR "Error al escribir el dato del registro\n");
        return ret_val;
    }

    /*Start*/

    if(ret_val = i2c_sitara_start(2) != 0)
    {
        printk(KERN_ERR "Error al iniciar el bus\n");
        return ret_val;
    }

    return 0;
}

/**
 * @brief 
 * 
 * @param slave_address 
 * @return int 
 */
int i2c_sitara_is_connected(uint8_t slave_address)
{

}

/**
 * @brief 
 * 
 * @param irq 
 * @param id 
 * @return irqreturn_t 
 */
irqreturn_t  i2c_sitara_irq_handler (int irq, void *dev id)
{
    uint32_t irq_status_raw = 0;

    printk(KERN_INFO "Interrupción del I2C2\n");

    irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);

    while(irq_status_raw != 0)
    {
        if(iowrite32(irq_status_raw, i2c2_registers+I2C_SITARA_IRQSTATUS) != 0)
        {
            printk(KERN_ERR "Error al limpiar las interrupciones del I2C2\n");
            return -EIO;
        }

        if((irq_status_raw & I2C_SITARA_RRDY_MASK) == I2C_SITARA_RRDY_MASK)
        {
            data_rdy = true;
            printk(KERN_INFO "Interrupción de recepción\n");
        }
        if((irq_status_raw & I2C_SITARA_XRDY_MASK) == I2C_SITARA_XRDY_MASK)
        {
            printk(KERN_INFO "Interrupción de transmisión\n");
        }
        if((irq_status_raw & I2C_SITARA_NACK_MASK) == I2C_SITARA_NACK_MASK)
        {
            printk(KERN_INFO "Interrupción de NACK\n");
        }
        if((irq_status_raw & I2C_SITARA_ARDY_MASK) == I2C_SITARA_ARDY_MASK)
        {
            printk(KERN_INFO "Interrupción de acceso a registro\n");
        }
        if((irq_status_raw & I2C_SITARA_BF_MASK) == I2C_SITARA_BF_MASK)
        {
            printk(KERN_INFO "Interrupción de bus libre\n");
        }


        irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);
    }
    
    return IRQ_HANDLED;
}



/// @brief Configura el clock del I2C2
/// @param void 
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
static int i2c_sitara_config_peripheral_clock(void)
{
    int ret_val = 0;
    
    void __iomem *cm_per = NULL;
    void __iomem *cm_wkup = NULL;

    /*Configuración de clock de periféricos segúnn el manual*/
    cm_per = ioremap(CM_PER_BASE, CM_PER_SIZE);
    if(cm_per == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de CM_PER\n");
        return -ENOMEM;
    }

    cm_wkup = ioremap(CM_WKUP_BASE, CM_WKUP_SIZE);
    if(cm_wkup == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de CM_WKUP\n");
        iounmap(cm_per);
        return -ENOMEM;
    }

    ret_val = iowrite32(0x2, cm_per+CM_PER_I2C2_CLKCTRL_OFFSET);

    if(ret_val)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    ret_val = iowrite32(0x4, cm_per+CM_PER_I2C2_CLKCTRL_OFFSET);

    if(ret_val)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    if(ret_val = pool_register(cm_wkp+CM_WKP_IDLEST_DPLL_PER_OFFSET, 0x1, 0x1) != 0)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    ret_val = iowrite32(0x0, cm_wkp+CM_WKP_CLKSEL_DPLL_PER_OFFSET);

    if(ret_val)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    ret_val = iowrite32(0x1, cm_wkp+CM_WKP_DIV_M2_DPLL_PER_OFFSET);

    if(ret_val)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    ret_val = iowrite32(0x7, cm_wkp+CM_WKP_CLKMODE_DPLL_PER_OFFSET);

    if(ret_val)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    if(ret_val = pool_register(cm_wkp+CM_WKP_IDLEST_DPLL_PER_OFFSET, 0x1, 0x1) != 0)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    iounmap(cm_per);
    iounmap(cm_wkup);

    printk(KERN_INFO "Configuración de clock i2c2 exitosa\n");
    return ret_val;
}


/// @brief       Realiza un pooling de un registro hasta que se cumpla una condición
/// @param reg   Dirección del registro a realizar el pooling
/// @param mask  Máscara de bits a comparar
/// @param value Valor a comparar
/// @param timeout Tiempo máximo en milisegundos de pooling
/// @return      -EIO if the pooling took too long, or the time it took to pool
static int pool_register(void __iomem *reg, uint32_t mask, uint32_t value, uint32_t timeout)
{
    unsigned int counter = 0;

    while((ioread32(reg) & mask) != value)
    {
        msleep(1); //Duerme el proceso por 1 ms (No bloqueante)

        counter++;
        if(counter > timeout)
        {
            printk(KERN_ERR "El pooling tardó demasiado\nNo se pudo configurar");
            return -EIO;
        }
    }

    printk(KERN_INFO "El pooling tardó %d ms\n", counter);

    return 0;
}

/// @brief Función que configura los pins del I2C2
/// @param  void
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
static int i2c_sitara_config_pinmux(void)
{
    printk(KERN_INFO "Configuración de pines i2c2 exitosa\n");
    return 0;
}

/**
 * @brief 
 * 
 * @return int 
 */
static int i2c_sitara_config_regs(void)
{
    int ret_val = 0;
    uint32_t aux = 0

    if(i2c2_registers == NULL)
    {
        i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE);

        if(i2c2_registers == NULL)
        {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        return -ENOMEM;
        }   
    }
    
    /*I2c_CON:I2C_EN =1*/
    /*Configure the I2C mode register (I2C_CON) bits.*/

    aux = ioread32(i2c2_registers+I2C_SITARA_CON);
    aux |= I2C_SITARA_CON_EN | I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX  ;/*| I2C_SITARA_CON_STT | I2C_SITARA_CON_STP ;*/

    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_CON))
    {
        printk(KERN_ERR "Error al configurar el registro I2C_SITARA_CON\n");
        return ret_val;
    }

    /*Enable interrupt masks (I2C_IRQENABLE_SET), if using interrupt for transmit/receive data.*/

    aux = ioread32(i2c2_registers+I2C_SITARA_IRQENABLE_SET);

    /* ARDY (Reg Ready to Access), RRDY (Receive Ready), XRDY (Transmit Ready), NACK (No Acknowledge), BF (Buss Free)*/
    aux |= I2C_SITARA_IRQENABLE_SET_ARDY | I2C_SITARA_IRQENABLE_SET_RRDY | I2C_SITARA_IRQENABLE_SET_XRDY | I2C_SITARA_IRQENABLE_SET_NACK | I2C_SITARA_IRQENABLE_SET_BF;

    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_IRQENABLE_SET))
    {
        printk(KERN_ERR "Error al configurar el registro I2C_SITARA_IRQENABLE_SET\n");
        return ret_val;
    }

    
    printk(KERN_INFO "Configuración de registros i2c2 exitosa\n");
    return ret_val;
}

/**
 * @brief 
 * 
 * @return int 
 */
static int i2c_sitara_config_interrupts(void)
{
    int ret_val = 0;

    if((ret_val = request_irq(IRQ_I2C2, i2c_sitara_irq_handler, IRQF_TRIGGER_RISING, NULL)) != 0)
    {
        printk(KERN_ERR "Error al solicitar la interrupción del I2C2\n");
        return ret_val;
    }

    printk(KERN_INFO "Configuración de interrupciones i2c2 exitosa\n");
    return ret_val;
}

/**
 * @brief 
 * 
 * @return int 
 */
static int i2c_sitara_free_interrupts(void)
{
    int ret_val = 0;

    if((ret_val = free_irq(IRQ_I2C2, NULL)) != 0)
    {
        printk(KERN_ERR "Error al liberar la interrupción del I2C2\n");
        return ret_val;
    }

    printk(KERN_INFO "Liberación de interrupciones i2c2 exitosa\n");
    return ret_val;
}

#define I2C_CLK_INT 48000000
#define I2C_CLK 12000000
#define I2C_BIT_RATE 100000

/**
 * @brief 
 * 
 * @return int 
 */
static int i2c_sitara_config_clock(void)
{
    int ret_val = 0;

    unsigned int aux = 0;


    if(i2c_sitara_registers == NULL)
    {
        i2c_sitara_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE);

        if(i2c_sitara_registers == NULL)
        {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        return -ENOMEM;
        }   
    }

    /*Configurando prescaler*/

    /*I2C_PSC = x para obtener 12MHz*/
    if(ret_val = iowrite32(0x3, i2c_sitara_registers+I2C_SITARA_PSC))
    {
        printk(KERN_ERR "Error al configurar el prescaler del I2C2\n");
        return ret_val;
    }

    /*Programar para obtener entre 100Kbps o 400Kbps con SCLL Y SCLH*/

    aux = I2C_CLK_INT /(I2C_BIT_RATE *2) - 7;
    if(ret_val = iowrite32(aux, i2c_sitara_registers+I2C_SITARA_SCLL))
    {
        printk(KERN_ERR "Error al configurar el SCLL del I2C2\n");
        return ret_val;
    }

    aux = I2C_CLK_INT /(I2C_BIT_RATE *2) - 5;
    if(ret_val = iowrite32(aux, i2c_sitara_registers+I2C_SITARA_SCLH))
    {
        printk(KERN_ERR "Error al configurar el SCLH del I2C2\n");
        return ret_val;
    }


    return 0;
}

/// @brief 
/// @param  
/// @return 
static int i2c_sitara_start(unsigned int dcount)
{
    int ret_val = 0;
    uint32_t aux = 0;
    
    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        printk(KERN_ERR "No se puede iniciar el bus\n");
        return -ENOMEM;
    }
    
    if(ret_val = iowrite32(dcount, i2c2_registers+I2C_SITARA_CNT) != 0)
    {
        printk(KERN_ERR "Error al escribir el registro de conteo\n");
        return ret_val;
    }

    aux = ioread32(i2c2_registers+I2C_SITARA_CON);
    aux |= I2C_SITARA_CON_STT | I2C_SITARA_CON_STP;

    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_CON) != 0)
    {
        printk(KERN_ERR "Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    
    return 0;
}

/// @brief 
/// @param  
/// @return   
static int i2c_reset_fifos(void)
{
    int ret_val = 0;
    uint32_t aux = 0;

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        printk(KERN_ERR "No se puede parar el bus\n");
        return -ENOMEM;
    }

    aux = ioread32(i2c2_registers+I2C_SITARA_BUF);

    aux |= I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR;

    if(ret_val = iowrite32(aux, i2c2_registers+I2C_SITARA_BUF) != 0)
    {
        printk(KERN_ERR "Error al resetear los FIFOs\n");
        return ret_val;
    }

    return 0;
}
