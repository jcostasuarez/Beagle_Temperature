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
#include "bmp280.h"
#include "bmp280_cdevice.h"
#include "i2c_sitara.h"
#include "utils.h"

/* Funciones secundarias, privadas */

static irqreturn_t  i2c_sitara_irq_handler (int , void *);

/* Variables globales, privadas */

/// @brief Variable que tiene mapeado los registros de i2c2
static void __iomem *i2c2_registers = NULL;

/*volatil*/

static volatile bool data_rdy = false;
static volatile int irq_number = 0;

static volatile bool bus_busy = false;
static volatile bool ardy = false;
static volatile bool rrdy = false;
static volatile bool xrdy = false;

static volatile int trx[2] = {0,0};
static volatile int trx_index = 0;


/******** Funciones públicas ********/

/**
 * @brief 
 * 
 * @return int 
 */
int i2c_sitara_init(void)
{
    unsigned int aux = 0;

    /*Configuro los registros del i2c*/
    i2c2_registers = ioremap(I2C_SITARA_I2C2_BASE, I2C_SITARA_I2C2_SIZE);

    /*Verifico que se haya podido mapear la memoria*/
    if(i2c2_registers == NULL)
    {
        printk(KERN_ERR "Error al mapear la memoria de i2c2_registers\n");
        iounmap(i2c2_registers);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "i2_sitara_init: i2c2_registers = %p\n", i2c2_registers);

    /*Configuro el clock del I2C2*/

    /* Apago el módulo*/

    iowrite32(0x0, i2c2_registers+I2C_SITARA_CON);

    /*Configurando prescaler*/
    //0x01;53;55
    //0x04;29;31
    //0x08;14;15
    //12;113;115

    /*I2C_PSC = x para obtener 12MHz*/
    iowrite32(0x01, i2c2_registers+I2C_SITARA_PSC);

    /*Programar para obtener entre 100Kbps o 400Kbps con SCLL Y SCLH*/
    aux = 53;
    printk(KERN_INFO "i2c_sitara_config_clock: scll = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLL);
    aux = 55;

    printk(KERN_INFO "i2c_sitara_config_clock: aux = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLH);

    /*Configumos direccion propia*/

    iowrite32(0xAA, i2c2_registers+I2C_SITARA_OA);

    /*Habilito el modulo*/

    iowrite32(I2C_SITARA_CON_EN | I2C_SITARA_CON_MST, i2c2_registers+I2C_SITARA_CON);


    // Habilito interrupciones

    iowrite32(I2C_SITARA_XRDY|I2C_SITARA_RRDY | I2C_SITARA_NACK | I2C_SITARA_ARDY | I2C_SITARA_AL, i2c2_registers+I2C_SITARA_IRQENABLE_SET);

    printk(KERN_INFO "i2c_sitara_config_clock: Configuración del clock del I2C2 exitosa\n");


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
 * @param data Puntero a la variable donde se guardará el dato
 * @return int 
 */
int i2c_sitara_read(uint32_t slave_address, uint32_t slave_register, uint32_t *data)
{
    int ret_val = 0;

    printk(KERN_INFO "i2c_sitara_read: Iniciando lectura\n");
    printk(KERN_INFO "i2c_sitara_read: slave_address = %x\n", slave_address);
    printk(KERN_INFO "i2c_sitara_read: slave_register = %x\n", slave_register);

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

    // pool bus
    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB, 0, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }

    // Set slave address
    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    //Configuro el contador
    iowrite32(1, i2c2_registers+I2C_SITARA_CNT);

    // Set slave register
    //trx[0]=slave_address;
    iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);

    // Reset FIFOs
    iowrite32(I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR, i2c2_registers+I2C_SITARA_BUF);

    xrdy = false;

    // Set master reciever mode and start
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_EN | I2C_SITARA_CON_TRX | I2C_SITARA_CON_STT |I2C_SITARA_CON_STP, i2c2_registers+I2C_SITARA_CON);

    ret_val = pool_bool(&xrdy, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    xrdy = false;

    ardy = false;

    iowrite32(1, i2c2_registers+I2C_SITARA_CNT);
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_EN | I2C_SITARA_CON_STT |I2C_SITARA_CON_STP, i2c2_registers+I2C_SITARA_CON);

    printk(KERN_INFO "i2c_sitara_read: Operacion de lectura reiniciada\n");

    //pool ardy
    ret_val = pool_bool(&ardy, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    ardy = false;

    /*Read buffer until its empty*/
    *data = ioread32(i2c2_registers+I2C_SITARA_DATA);

    printk(KERN_INFO "i2c_sitara_read: data = %d\n", *data);

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
int i2c_sitara_write(const uint32_t slave_address, const uint32_t slave_register, const uint32_t data)
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

    //Configuro el contador
    iowrite32(2, i2c2_registers+I2C_SITARA_CNT);

    // Set slave address
    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    // Set slave register
    iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);

    // Set data
    iowrite32(data, i2c2_registers+I2C_SITARA_DATA);

    // Set master reciever mode and start
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX | I2C_SITARA_CON_EN | I2C_SITARA_CON_STT | I2C_SITARA_CON_STP, i2c2_registers+I2C_SITARA_CON);

    ardy = false;
    // pool ardy
    ret_val = pool_bool(&ardy, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    ardy = false;

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
    uint32_t aux = 0;

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c: Registros no mapeados en memoria, iniciar el bus\n");
    }

    ret_val = i2c_sitara_read(slave_address, 0xD0, &aux);

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
    uint32_t irq_status = 0;
    uint32_t irq_status_raw = 0;

    printk(KERN_INFO "i2c_sitara_irq_handler: IRQ\n");

    irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);

    //irq_status = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);

    printk(KERN_INFO "i2c_sitara_irq_handler: irq_status_raw = %x\n", irq_status_raw);
    
    irq_status = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);

    printk(KERN_INFO "i2c_sitara_irq_handler: irq_status = %x\n", irq_status);

    while((irq_status = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS))!= 0)
    {
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
            printk(KERN_INFO "i2c_sitara_irq_handler: Operación de lectura/escritura finalizada\n");
            ardy = true;
        }
        if(irq_status_raw & I2C_SITARA_RRDY)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_RRDY\n");
            printk(KERN_INFO "i2c_sitara_irq_handler: Dato listo para leer\n");
            rrdy = true;
        }
        if(irq_status_raw & I2C_SITARA_XRDY)
        {
            printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XRDY\n");
            printk(KERN_INFO "i2c_sitara_irq_handler: Dato listo para escribir\n");
            xrdy = true;
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
        
        iowrite32(0x6FFF, i2c2_registers+I2C_SITARA_IRQSTATUS);

    }
    
    printk(KERN_INFO "i2c_sitara_irq_handler: IRQ handled\n");

    return IRQ_HANDLED;
}


/// @brief Configura el clock del I2C2 y enciende el módulo
/// @param void 
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
int i2c_sitara_turn_on_peripheral(void)
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

    iowrite32( ioread32(cm_per+CM_PER_I2C2_CLKCTRL_OFFSET) | 0x2, cm_per+CM_PER_I2C2_CLKCTRL_OFFSET);

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

    printk(KERN_INFO "i2c_sitara_turn_on_peripheral: Configuración del clock del I2C2 exitosa\n");
    
    return 0;

}


/// @brief Función que configura los pins del I2C2
/// @param  void
/// @return 0 si la configuración fue exitosa o un valor negativo en caso de error
int i2c_sitara_config_pinmux(void)
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
int i2c_sitara_config_interrupts(struct platform_device *pdev)
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
 * @brief  free_irq() - free an interrupt allocated with request_irq()
 * 
 * @return int 
 */
int i2c_sitara_free_interrupts(void)
{

    free_irq(irq_number, NULL);

    printk(KERN_INFO "i2c_sitara_free_interrupts: free_irq() OK!\n");

    return 0;
}

/*
static int i2c_sitara_print_status(void)
{
    //print all the registers

    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c_sitara_print_status: NULL ptr\n");
        return -ENOMEM;
    }

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

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_CON \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_CON));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_OA \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_OA));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SA \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SA));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_PSC \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_PSC));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SCLL \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SCLL));

    printk(KERN_INFO "i2c_sitara_print_status: I2C_SITARA_SCLH \t= 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_SCLH));

    return 0;
}
*/