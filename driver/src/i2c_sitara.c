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
static int i2c_sitara_start(void);
static int i2c_sitara_reset(void);

static int i2c_sitara_turn_on_peripheral(void);
static int i2c_sitara_config_pinmux(void);
static int i2c_sitara_config_interrupts(struct platform_device *pdev);
static int i2c_sitara_config_clock(void);
static int i2c_sitara_config_regs(void);

static int i2c_sitara_free_interrupts(void);

static int i2c_reset_fifos(void);
static int i2c_sitara_set_count(unsigned int dcount);
static int i2c_sitara_print_status(void);

static int pool_register(void *reg, uint32_t mask, uint32_t value, uint32_t timeout);

static irqreturn_t  i2c_sitara_irq_handler (int , void *);

/* Variables globales, privadas */

/// @brief Variable que tiene mapeado los registros de i2c2
static void __iomem *i2c2_registers = NULL;

/*volatil*/

static volatile bool data_rdy = false;
static volatile int irq_number = 0;

static volatile int global_data[2] = {0,0};

static volatile int recieve_ready = 0;
static volatile int transmit_ready = 0;

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

    if((ret_val = i2c_sitara_reset()) != 0)
    {
        printk( KERN_ERR "Error al resetear el I2C2\n");
        iounmap(i2c2_registers);
        return ret_val;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_reset() OK!\n" );




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

    return 0;
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

        return -ENOMEM;
    }

    printk(KERN_INFO "i2c_sitara_exit: i2c2_registers unmapped\n");

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

    printk(KERN_INFO "i2c_sitara_read: Iniciando lectura\n");
    printk(KERN_INFO "i2c_sitara_read: slave_address = %x\n", slave_address);
    printk(KERN_INFO "i2c_sitara_read: slave_register = %x\n", slave_register);
    printk(KERN_INFO "i2c_sitara_read: mask = %x\n", mask);

    if(data == NULL)
    {
        printk(KERN_ERR "i2c_sitara_read: data = NULL\n");
        return -ENOMEM;
    }

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_read: Registros no mapeados en memoria, iniciar el bus\n");
        return -ENOMEM;
    }

    // Set slave address

    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    // Set data to send

    global_data[0] = slave_register;

    iowrite32(global_data[0], i2c2_registers+I2C_SITARA_DATA);

    // pool bus
    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB, 0, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }

    printk(KERN_INFO "i2c_sitara_read: Pool: OK!\n" );

    /*Reset FIFOs*/
    if(i2c_reset_fifos())
    {
        printk(KERN_ERR "i2c_sitara_read: Error al resetear los FIFOs\n");
        return -1;
    }

    printk(KERN_INFO "i2c_sitara_read: FIFOs reseteados\n" );
    
    if(i2c_sitara_set_count(1))
    {
        printk(KERN_ERR "i2c_sitara_read: Error al configurar el contador\n");
        return -1;
    }

    //Set interrupt mask
    
    iowrite32(I2C_SITARA_RRDY, i2c2_registers+I2C_SITARA_IRQENABLE_SET);

    //Set maste reciver mode

    iowrite32(I2C_SITARA_CON_EN| I2C_SITARA_CON_MST, i2c2_registers+I2C_SITARA_CON);

    printk(KERN_INFO "i2c_sitara_read: Contador configurado\n" );


    /*Start*/
    ret_val = i2c_sitara_start();

    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al iniciar el bus\n");
        return ret_val;
    }

    printk(KERN_INFO "i2c_sitara_read: i2c_sitara_start() OK!\n" );

    // pool bus
    //ret_val = pool_register(&recieve_ready, 1, 1, 1000);
    //if(ret_val != 0)
    //{
    //    printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
    //    return ret_val;
    //}
    //recieve_ready = 0;

    msleep(4);

    printk(KERN_INFO "i2c_sitara_read: Pool: OK!\n" );

    /*Read buffer*/

    *data = global_data[0];

    *data = *data & mask;

    printk(KERN_INFO "i2c_sitara_read: masked data = %x\n", *data);

    printk(KERN_INFO "i2c_sitara_read: Lectura exitosa\n");

    return 0;
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

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_write: Registros no mapeados en memoria, iniciar el bus\n");
        return -ENOMEM;
    }

    // pool bus
    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB, 0, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    
    if(i2c_sitara_set_count(2))
    {
        printk(KERN_ERR "i2c_sitara_read: Error al configurar el contador\n");
        return -1;
    }

    /*Reset FIFOs*/
    if(i2c_reset_fifos())
    {
        printk(KERN_ERR "i2c_sitara_read: Error al resetear los FIFOs\n");
        return -1;
    }

    /*Set slave address*/
    iowrite32(slave_address << 1, i2c2_registers+I2C_SITARA_SA);
    /*Set data*/
    iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);
    iowrite32(data, i2c2_registers+I2C_SITARA_DATA);

    /*Start*/
    ret_val = i2c_sitara_start();
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al iniciar el bus\n");
        return ret_val;
    }

    // pool bus
    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB, 0, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
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

    ret_val = i2c_sitara_read(slave_address, 0xD0, 0xff, &aux);

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

    irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);

    printk(KERN_INFO "i2c_sitara_irq_handler: irq_status_raw = %d\n", irq_status_raw);

    while(irq_status_raw != 0)
    {
        set_bit_32(i2c2_registers+I2C_SITARA_IRQSTATUS, irq_status_raw);

        if(irq_status_raw & I2C_SITARA_AL)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_AL\n");
        }
        if(irq_status_raw & I2C_SITARA_NACK)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_NACK\n");
        }
        if(irq_status_raw & I2C_SITARA_ARDY)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_ARDY\n");
            i2c_reset_fifos();
            // Configure in master receive mode
            iowrite32(I2C_SITARA_CON_EN| I2C_SITARA_CON_MST, i2c2_registers+I2C_SITARA_CON);
            printk(KERN_INFO "i2c_sitara_irq_handler: slave address: %d\n", ioread32(i2c2_registers+I2C_SITARA_SA));

            i2c_sitara_start();   
        }
        if(irq_status_raw & I2C_SITARA_RRDY)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_RRDY\n");

            // Get data
            global_data[0] = ioread32(i2c2_registers+I2C_SITARA_DATA);

            printk(KERN_INFO "i2c_sitara_irq_handler: data = %x\n", global_data[0]);

            //deshabilito la interrupción de RRDY

            iowrite32(I2C_SITARA_RRDY, i2c2_registers+I2C_SITARA_IRQENABLE_CLR);
            
            recieve_ready = 1;

        }
        if(irq_status_raw & I2C_SITARA_XRDY)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XRDY\n");

            // Set data
            iowrite32(global_data[0], i2c2_registers+I2C_SITARA_DATA);

            printk(KERN_INFO "i2c_sitara_irq_handler: data = %x\n", global_data[0]);

            //deshabilito la interrupción de XRDY
            iowrite32(I2C_SITARA_XRDY, i2c2_registers+I2C_SITARA_IRQENABLE_CLR);
            //habilito la interrupción de RRDY
            iowrite32(I2C_SITARA_RRDY, i2c2_registers+I2C_SITARA_IRQENABLE_SET);

        }
        if(irq_status_raw & I2C_SITARA_GC)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_GC\n");
        }
        if(irq_status_raw & I2C_SITARA_STC)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_STC\n");
        }
        if(irq_status_raw & I2C_SITARA_AERR)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_AERR\n");
        }
        if(irq_status_raw & I2C_SITARA_BF)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_BF\n");
        }
        if(irq_status_raw & I2C_SITARA_AAS)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_AAS\n");
        }
        if(irq_status_raw & I2C_SITARA_XUDF)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XUDF\n");
        }
        if(irq_status_raw & I2C_SITARA_ROVR)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_ROVR\n");
        }
        if(irq_status_raw & I2C_SITARA_BB)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_BB\n");
        }
        if(irq_status_raw & I2C_SITARA_RDR)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_RDR\n");
        }
        if(irq_status_raw & I2C_SITARA_XDR)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XDR\n");
        }
        
        irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);
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
        printk(KERN_ERR "i2c_sitara_turn_on_peripheral: Error al mapear la memoria de CM_PER\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "i2c_sitara_turn_on_peripheral: cm_per = %p\n", cm_per);

    /*Reservo memoria para los registros de control*/
    cm_wkup = ioremap(CM_WKP_BASE, CM_WKP_SIZE);
    if(cm_wkup == NULL)
    {
        printk(KERN_ERR "i2c_sitara_turn_on_peripheral: Error al mapear la memoria de CM_WKP\n");
    iounmap(cm_per);
    return -ENOMEM;
    }
    
    printk(KERN_INFO "i2c_sitara_turn_on_peripheral: cm_wkup = %p\n", cm_wkup);

    /*Enciendo periferico*/
    /*Lo coloco en modo activado*/
    /*Página 1270*/
    set_bit_32(cm_per+CM_PER_I2C2_CLKCTRL_OFFSET, 0x2);

    ret_val = pool_register(cm_per+CM_PER_I2C2_CLKCTRL_OFFSET,  0x03, 0x02,1000);

    if( ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_turn_on_peripheral: Error al configurar el clock del I2C2\n");
        iounmap(cm_per);
        iounmap(cm_wkup);
        return ret_val;
    }

    /*Condiguro el registro del divisorM2 del DPLL*/
    
    /*Página 1361*/
    //iowrite32(0x0, cm_wkup+CM_WKP_CLKSEL_DPLL_PER_OFFSET);

    /*Página 1365*/
    //iowrite32(0x1, cm_wkup+CM_WKP_DIV_M2_DPLL_PER_OFFSET);

    /*Página 1354*/
    //iowrite32(0x7, cm_wkup+CM_WKP_CLKMODE_DPLL_PER_OFFSET);
    
    iounmap(cm_per);
    iounmap(cm_wkup);

    msleep(10);
    printk(KERN_INFO "i2c_sitara_turn_on_peripheral: Configuración del clock del I2C2 exitosa\n");
    
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

    printk(KERN_INFO "pool_register: Iniciando pooling\n");

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

    printk(KERN_INFO "pool_register: Pooling exitoso\n");

    printk(KERN_INFO "pool_register: Tiempo de espera: %d\n", counter);

    return 0;
}

/// @brief Función que configura los pins del I2C2
/// @param  void
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
static int i2c_sitara_config_pinmux(void)
{
    static void __iomem *ctrl_module = NULL;

    printk(KERN_INFO "i2c_sitara_config_pinmux: Configurando los pines del I2C2\n");

    ctrl_module = ioremap(CTRL_MODULE_BASE, CTRL_MODULE_SIZE);

    if (ctrl_module == NULL)
    {
        printk(KERN_ERR "i2c_sitara_config_pinmux: Error al mapear la memoria de CTRL_MODULE_BASE\n");
        return 1;
    }

    printk(KERN_INFO "i2c_sitara_config_pinmux: ctrl_module = %p\n", ctrl_module);

    printk(KERN_INFO "i2c_sitara_config_pinmux: PIN 20 : UART1_Ctsn - I2C2_SDA\n");

    iowrite32(PIN_I2C_CFG, ctrl_module + CONF_UART1_CSTN);

    printk(KERN_INFO "i2c_sitara_config_pinmux: PIN 19 : UART1_rtsn - I2C2_SCL\n");

    iowrite32(PIN_I2C_CFG, ctrl_module + CONF_UART1_RSTN);

    printk(KERN_INFO "i2c_sitara_config_pinmux: Pines configurados correctamente\n");

    iounmap(ctrl_module);

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
    iowrite32(I2C_SITARA_CON_EN| I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX,i2c2_registers+I2C_SITARA_CON);

    /*Enable interrupt masks (I2C_IRQENABLE_SET), if using interrupt for transmit/receive data.*/
    iowrite32(0xffff,i2c2_registers+I2C_SITARA_IRQENABLE_SET);//I2C_SITARA_ARDY | I2C_SITARA_RRDY | I2C_SITARA_XRDY | I2C_SITARA_NACK |I2C_SITARA_AL | I2C_SITARA_BF);

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

    printk(KERN_INFO "i2c_sitara_config_interrupts: Configuración de interrupciones i2c2 exitosa\n");

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

    printk(KERN_INFO "i2c_sitara_free_interrupts: free_irq() OK!\n");

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
        printk(KERN_ERR "i2c_sitara_config_clock: Los registros no están mapeados\n");
    }

    /*Configurando prescaler*/

    /*I2C_PSC = x para obtener 12MHz*/
    iowrite32(12, i2c2_registers+I2C_SITARA_PSC);

    /*Programar para obtener entre 100Kbps o 400Kbps con SCLL Y SCLH*/

    aux = 113;

    printk(KERN_INFO "i2c_sitara_config_clock: scll = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLL);

    aux = 115;

    printk(KERN_INFO "i2c_sitara_config_clock: aux = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLH);

    printk(KERN_INFO "i2c_sitara_config_clock: Configuración del clock del I2C2 exitosa\n");

    return ret_val;
}

/// @brief 
/// @param  
/// @return 
static int i2c_sitara_start(void)
{
    int ret_val = 0;

    iowrite32(0xFF, i2c2_registers+I2C_SITARA_IRQENABLE_SET);
    
    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_start: NULL ptr\n");
        return -ENOMEM;
    }

    /*Configuro los bits de control*/
    iowrite32( I2C_SITARA_CON_STT | I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX | I2C_SITARA_CON_EN , i2c2_registers+I2C_SITARA_CON);
    
    printk(KERN_INFO "i2c_sitara_start: i2c2_registers start\n");

    return ret_val;
}

static int i2c_sitara_set_count(unsigned int dcount)
{
    int ret_val = 0;

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_set_count: NULL ptr\n");
        return -ENOMEM;
    }

    /*Seteo la cuenta de bits*/
    iowrite32(dcount, i2c2_registers+I2C_SITARA_CNT);

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
        printk(KERN_ERR "i2c_reset_fifos: NULL ptr\n");
        return -ENOMEM;
    }

    iowrite32(I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR, i2c2_registers+I2C_SITARA_BUF);

    return ret_val;
}


static int i2c_sitara_reset(void)
{
    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_reset: NULL ptr\n");
        return -ENOMEM;
    }

    iowrite32(0xffff, i2c2_registers+I2C_SITARA_IRQENABLE_CLR);

    iowrite32(I2C_SITARA_SYSC_SRST, i2c2_registers+I2C_SITARA_SYSC);

    printk(KERN_INFO "i2c_sitara_reset: i2c2_registers reset\n");

    return 0;
}

static int i2c_sitara_print_status(void)
{
    //print all the registers

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_REVNB_LO \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_REVNB_LO));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_REVNB_HI \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_REVNB_HI));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SYSC \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SYSC));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_IRQSTATUS_RAW \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_IRQSTATUS \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_IRQENABLE_SET \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQENABLE_SET));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_IRQENABLE_CLR \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQENABLE_CLR));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_WE \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_WE));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SYSS \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SYSS));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_BUF \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_BUF));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_CNT \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_CNT));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_DATA \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_DATA));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_CON \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_CON));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SA \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SA));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_PSC \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_PSC));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SCLL \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SCLL));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SCLH \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SCLH));

    return 0;
}