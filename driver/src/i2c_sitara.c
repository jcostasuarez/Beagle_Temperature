/**
 * @file i2c_sitara.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief Este módulo contiene las funciones para el manejo del bus I2C2 del Sitara
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/*Funciones principales*/
#include "i2c_sitara.h"
#include "bmp280_cdevice.h"
#include "utils.h"

/* Funciones secundarias, privadas */

static int i2c_sitara_turn_on_peripheral(void);
static int i2c_sitara_config_pinmux(void);
static int i2c_sitara_config_regs(void);
static int i2c_sitara_config_interrupts(struct platform_device *pdev);
static int i2c_sitara_free_interrupts(void);
static int i2c_sitara_config_clock(void);
static int i2c_sitara_start(unsigned int dcount);
static int i2c_reset_fifos(void);
static int pool_register(void __iomem *reg, uint32_t mask, uint32_t value, uint32_t timeout);

static irqreturn_t  i2c_sitara_irq_handler (int , void *);

/* Variables globales, privadas */

/// @brief Variable que tiene mapeado los registros de i2c2
static void __iomem *i2c2_registers = NULL;


/*volatil*/

static volatile bool data_rdy = false;
static volatile int irq_number = 0;

/******** Funciones públicas ********/

/**
 * @brief 
 * 
 * @return int 
 */
int i2c_sitara_init(struct platform_device *pdev)
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
    
    printk(KERN_INFO "i2_sitara_init: i2c2_registers = %p\n", i2c2_registers);

    /*Configuro los pines del I2C2*/
    
    if((ret_val = i2c_sitara_config_pinmux()) != 0)
    {
        printk( KERN_ERR "Error al configurar los pines del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_pinmux() OK!\n" );

    if((ret_val = i2c_sitara_turn_on_peripheral()) != 0)
    {
        printk( KERN_ERR "Error al configurar el periferico del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_turn_on_peripheral() OK!\n" );

    /*Configuro el clock del I2C2*/

    if((ret_val = i2c_sitara_config_clock()))
    {
        printk( KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_clock() OK!\n" );

    /*Configuro las interrupciones del I2C2*/

    if((ret_val = i2c_sitara_config_interrupts(pdev)) != 0)
    {
        printk( KERN_ERR "Error al configurar las interrupciones del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }
    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_interrupts() OK!\n" );

    /*Configuro los registros del I2C2 y habilito el bus*/
    ret_val = i2c_sitara_config_regs();

    if(ret_val != 0)
    {
        printk( KERN_ERR "Error al configurar los registros del I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_regs() OK!\n" );

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

    i2c_sitara_free_interrupts();

    if(i2c2_registers!=NULL)
    {
        iounmap(i2c2_registers);
    }

    printk(KERN_INFO "Liberación de interrupciones i2c2 exitosa\n");
    return ret_val;
}

/**
 * @brief Esta función lee un registro de un esclavo i2c, usando el bus i2c2, del Sitara y lo guarda en data antes de pasar por la máscara
 * @param slave_address Dirección del esclavo
 * @param slave_register Dirección del registro a leer
 * @param mask Máscara de bits a aplicar
 * @param data Puntero a la variable donde se guardará el dato
 * @return int 
 */
int i2c_sitara_read(uint8_t slave_address, uint8_t slave_register, uint8_t mask, uint8_t *data)
{
    int ret_val = 0;
    uint32_t aux = 0;
    unsigned int timeout = 0;

    if(i2c2_registers==NULL)
    {
        if((i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE)) == NULL)
        {
            printk(KERN_ERR "i2c_sitara_read: Error al mapear la memoria de i2c2_registers\n");
            return -ENOMEM;
        }
    }

    /* Pool del bit BB del registro I2C_IRQSTATUS_RAW*/
    //ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB_MASK, I2C_SITARA_IRQENABLE_SET_BB, 500);
    //if(ret_val != 0)
    //{
    //    printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
    //    return ret_val;
    //}

    msleep(10);

    /*Reset FIFOs*/
    ret_val=i2c_reset_fifos();

    if( ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al resetear los FIFOs\n");
        return ret_val;
    }

    /*Set slave address*/

    aux = (slave_address << 1) | 0x1;
    
    iowrite32(aux, i2c2_registers+I2C_SITARA_SA);

    /*Set data*/

    iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);

    /*Start*/
    ret_val = i2c_sitara_start(1);

    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al iniciar el bus\n");
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
    
    if(timeout == 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Timeout\n");
        //return -EIO;
    }

    printk(KERN_INFO "i2c_sitara_read: tiempo de espera: %d\n", 1000 - timeout);

    /*Read buffer until its empty*/

    *data = ioread32(i2c2_registers+I2C_SITARA_DATA);

    printk(KERN_INFO "i2c_sitara_read: raw data = %d\n", *data);

    *data = *data & mask;

    printk(KERN_INFO "i2c_sitara_read: masked data = %d\n", *data);

    printk(KERN_INFO "i2c_sitara_read: Lectura exitosa\n");

    *data = ioread32(i2c2_registers+I2C_SITARA_DATA);

    printk(KERN_INFO "i2c_sitara_read: raw data = %d\n", *data);

    *data = *data & mask;

    printk(KERN_INFO "i2c_sitara_read: masked data = %d\n", *data);

    printk(KERN_INFO "i2c_sitara_read: Lectura exitosa\n");

    *data = ioread32(i2c2_registers+I2C_SITARA_DATA);

    printk(KERN_INFO "i2c_sitara_read: raw data = %d\n", *data);

    *data = *data & mask;

    printk(KERN_INFO "i2c_sitara_read: masked data = %d\n", *data);

    printk(KERN_INFO "i2c_sitara_read: Lectura exitosa\n");

    return ret_val;
}

/**
 * @brief Esta función escribe un registro de un esclavo i2c, usando el bus i2c2, del Sitara
 * @param slave_address Dirección del esclavo
 * @param slave_register Dirección del registro a escribir
 * @param data Dato a escribir
 * @return int 
 */
int i2c_sitara_write(uint8_t slave_address, uint8_t slave_register,  uint8_t data)
{
    int ret_val = 0;
    uint32_t aux = 0;

    if(i2c2_registers==NULL)
    {
        if((i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE)) == NULL)
        {
            printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
            return -ENOMEM;
        }
    }

    /* Pool del bit BB del registro I2C_IRQSTATUS_RAW*/

    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB_MASK, I2C_SITARA_IRQENABLE_SET_BB, 1000);

    if(ret_val != 0)
    {
        printk(KERN_ERR "Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }

    /*Reset FIFOs*/

    ret_val=i2c_reset_fifos();

    if(ret_val != 0)
    {
        printk(KERN_ERR "Error al resetear los FIFOs\n");
        return ret_val;
    }

    /*Set slave address*/

    aux = (slave_address << 1) | 0x0;
    iowrite32(aux, i2c2_registers+I2C_SITARA_SA);

    /*Write Data*/
    iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);

    iowrite32(data, i2c2_registers+I2C_SITARA_DATA);

    /*Start*/
    ret_val = i2c_sitara_start(2);
    if( ret_val!= 0)
    {
        printk(KERN_ERR "Error al iniciar el bus\n");
        return ret_val;
    }

    return 0;
}

/**
 * @brief 
 * @param slave_address 
 * @return int 
 */
int i2c_sitara_is_connected(uint8_t slave_address)
{
    int ret_val = 0;
    uint8_t aux = 0;

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c: Registros no mapeados en memoria, iniciar el bus\n");
    }

    ret_val = i2c_sitara_read(slave_address, 0xD0, 0x0, &aux);

    if( ret_val != 0)
    {
        printk(KERN_ERR "i2c: Error al leer el registro\n");
        return ret_val;
    }

    return 0;
}

/**
 * @brief 
 * 
 * @param irq 
 * @param id 
 * @return irqreturn_t 
 */
static irqreturn_t  i2c_sitara_irq_handler (int irq, void *dev_id)
{
    uint32_t irq_status_raw = 0;

    printk(KERN_INFO "i2c_sitara_irq_handler: IRQ\n");

    irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);

    while(irq_status_raw != 0)
    {
        set_bit_32(i2c2_registers+I2C_SITARA_IRQSTATUS, irq_status_raw);

        if((irq_status_raw & I2C_SITARA_RRDY_MASK))
        {
            data_rdy = true;
            printk(KERN_INFO "i2c_sitara_irq_handler: Receive data ready for read (RX FIFO threshold reached)\n");
        }
        if((irq_status_raw & I2C_SITARA_XRDY_MASK))
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: Transmit data ready\n");
        }
        if((irq_status_raw & I2C_SITARA_NACK_MASK))
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: Not Acknowledge detected\n");
        }
        if((irq_status_raw & I2C_SITARA_ARDY_MASK))
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: Access ready\n");
        }
        if((irq_status_raw & I2C_SITARA_BF_MASK))
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: Bus Free\n");
        }

        irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);
    }
    
    printk(KERN_INFO "i2c_sitara_irq_handler: IRQ handled\n");

    return IRQ_HANDLED;
}


/// @brief Configura el clock del I2C2
/// @param void 
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
static int i2c_sitara_turn_on_peripheral(void)
{
    int ret_val = 0;
    
    void __iomem *cm_per = NULL;
    void __iomem *cm_wkup = NULL;

    /*Reservo memoria para los registros de control*/
    cm_per = ioremap(CM_PER_BASE, CM_PER_SIZE);
    if(cm_per == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de CM_PER\n");
        return -ENOMEM;
    }
    /*Reservo memoria para los registros de control*/
    cm_wkup = ioremap(CM_WKP_BASE, CM_WKP_SIZE);
    if(cm_wkup == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de CM_WKUP\n");
        iounmap(cm_per);
        return -ENOMEM;
    }
    
    set_bit_32(cm_per+CM_PER_I2C2_CLKCTRL_OFFSET, 0x2|0x4);

    ret_val = pool_register(cm_wkup+CM_WKP_IDLEST_DPLL_PER_OFFSET, 0x1, 0x1,1000);

    if( ret_val != 0)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    /*Configuro ...*/
    set_bit_32(cm_wkup+CM_WKP_CLKSEL_DPLL_PER_OFFSET, 0x0);
    set_bit_32(cm_wkup+CM_WKP_DIV_M2_DPLL_PER_OFFSET, 0x1);
    set_bit_32(cm_wkup+CM_WKP_CLKMODE_DPLL_PER_OFFSET, 0x7);

    
    ret_val = pool_register(cm_wkup+CM_WKP_IDLEST_DPLL_PER_OFFSET, 0x1, 0x1,1000);
    if( ret_val != 0)
    {
        printk(KERN_ERR "Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    printk(KERN_INFO "Configuración del periferico exitosa\n");
    
    return 0;

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
            printk(KERN_ERR "pool_register: El pooling tardó demasiado.");
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
    // TODO, codigo ejemplo

    // Configure P9.21 and P9.22 pinmux as I2C
    //iowrite32(pins[1], control_module_ptr + pins[0]);
    //iowrite32(pins[3], control_module_ptr + pins[2]);


    printk(KERN_INFO "TODO\n");
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

    if(i2c2_registers == NULL)
    {
        printk(KERN_ERR "i2c_sitara_config_regs: Los registros no están mapeados\n");
 
    }
    
    /*I2c_CON:I2C_EN =1*/
    /*Configure the I2C mode register (I2C_CON) bits.*/
    set_bit_32(i2c2_registers+I2C_SITARA_CON, I2C_SITARA_CON_EN| I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX);

    /*Enable interrupt masks (I2C_IRQENABLE_SET), if using interrupt for transmit/receive data.*/
    set_bit_32(i2c2_registers+I2C_SITARA_IRQENABLE_SET, I2C_SITARA_IRQENABLE_SET_ARDY | I2C_SITARA_IRQENABLE_SET_RRDY | I2C_SITARA_IRQENABLE_SET_XRDY | I2C_SITARA_IRQENABLE_SET_NACK );
    
    printk(KERN_INFO "i2c_sitara_config_regs: Configuración de registros i2c2 exitosa\n");

    return ret_val;
}

/**
 * @brief 
 * 
 * @return int 
 */
static int i2c_sitara_config_interrupts(struct platform_device *pdev)
{
    int ret_val = 0;

    irq_number = platform_get_irq(pdev, 0);

    if(irq_number <= 0)
    {
        printk(KERN_ERR "i2c_sitara_config_interrupts: Error al obtener el irq\n");
        return -1;
    }

    printk(KERN_INFO "i2c_sitara_config_interrupts: irq_number = %d\n", irq_number);

    if((ret_val = request_irq(irq_number, i2c_sitara_irq_handler, 0, DEVICE_NAME, NULL)) != 0)
    {
        printk(KERN_ERR "Error al solicitar la interrupción\n");
        return ret_val;
    }

    printk(KERN_INFO "i2c_sitara_config_interrupts: request_irq() OK!\n");

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

    free_irq(irq_number, NULL);

    printk(KERN_INFO "Liberación de interrupciones i2c2 exitosa\n");

    return 0;
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

    if(i2c2_registers == NULL)
    {
        i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE);

        if(i2c2_registers == NULL)
        {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        return -ENOMEM;
        }   
    }

    /*Configurando prescaler*/

    /*I2C_PSC = x para obtener 12MHz*/
    iowrite32(0x3, i2c2_registers+I2C_SITARA_PSC);

    /*Programar para obtener entre 100Kbps o 400Kbps con SCLL Y SCLH*/

    aux = I2C_CLK_INT /(I2C_BIT_RATE *2) - 7;

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLL);

    aux = I2C_CLK_INT /(I2C_BIT_RATE *2) - 5;

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLH);

    return ret_val;
}

/// @brief 
/// @param  
/// @return 
static int i2c_sitara_start(unsigned int dcount)
{
    int ret_val = 0;

    iowrite32(0xFF, i2c2_registers+I2C_SITARA_IRQENABLE_CLR);
    
    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        printk(KERN_ERR "No se puede iniciar el bus\n");
        return -ENOMEM;
    }

    /*Seteo la cuenta de bits*/
    iowrite32(dcount, i2c2_registers+I2C_SITARA_CNT);

    /*Configuro los bits de control*/
    set_bit_32(i2c2_registers+I2C_SITARA_CON, I2C_SITARA_CON_STT | I2C_SITARA_CON_STP | I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX | I2C_SITARA_CON_EN);
    
    return ret_val;
}

/// @brief 
/// @param  
/// @return   
static int i2c_reset_fifos(void)
{
    int ret_val = 0;

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        printk(KERN_ERR "No se puede parar el bus\n");
        return -ENOMEM;
    }

    set_bit_32(i2c2_registers+I2C_SITARA_BUF, I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR);

    return ret_val;
}


