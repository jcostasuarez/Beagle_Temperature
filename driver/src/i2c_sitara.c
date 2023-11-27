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
<<<<<<< HEAD
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
=======
>>>>>>> tmp

static irqreturn_t  i2c_sitara_irq_handler (int , void *);

/* Variables globales, privadas */

/// @brief Variable que tiene mapeado los registros de i2c2
static void __iomem *i2c2_registers = NULL;


#define MAX_TIMEOUT 1000

#define MAX_SIZE_BUFFER 2

/*volatil*/

static volatile int irq_number = 0;

<<<<<<< HEAD
static volatile int global_data[2] = {0,0};

static volatile int recieve_ready = 0;
static volatile int transmit_ready = 0;
=======
uint32_t * trx;
volatile int trx_count = 0;

uint32_t * rx;
volatile int rx_count = 0;

DEFINE_MUTEX(lock_bus);
DECLARE_COMPLETION(ardy);
DECLARE_COMPLETION(rrdy);
DECLARE_COMPLETION(xrdy);
>>>>>>> tmp

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

<<<<<<< HEAD
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




=======
>>>>>>> tmp
    /*Configuro el clock del I2C2*/

    /* Apago el módulo*/

    iowrite32(0x0, i2c2_registers+I2C_SITARA_CON);

    /*Configurando prescaler*/
    //0x01;53;55

    /*I2C_PSC = x para obtener 12MHz*/
    iowrite32(0x01, i2c2_registers+I2C_SITARA_PSC);

    /*Programar para obtener entre 100Kbps o 400Kbps con SCLL Y SCLH*/
    aux = 53;
    printk(KERN_INFO "i2c_sitara_config_clock: scll = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLL);
    aux = 55;

    printk(KERN_INFO "i2c_sitara_config_clock: sclh = %d\n", aux);

    iowrite32(aux, i2c2_registers+I2C_SITARA_SCLH);

    /*Configumos direccion propia*/

    iowrite32(0xAA, i2c2_registers+I2C_SITARA_OA);

    /*Habilito interrupciones*/
    iowrite32(I2C_SITARA_XRDY|I2C_SITARA_RRDY | I2C_SITARA_NACK | I2C_SITARA_ARDY | I2C_SITARA_AL, i2c2_registers+I2C_SITARA_IRQENABLE_SET);

    //iowrite32(0xFFFF, i2c2_registers+I2C_SITARA_IRQENABLE_SET);

    /*Habilito el modulo*/

    iowrite32(I2C_SITARA_CON_EN | I2C_SITARA_CON_MST, i2c2_registers+I2C_SITARA_CON);

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_regs() OK!\n" );

<<<<<<< HEAD
=======
    //Solicito espacio para los vectores de recepción y transmisioon

    trx = kmalloc(MAX_SIZE_BUFFER*sizeof(uint32_t), GFP_KERNEL);
    if(trx == NULL)
    {
        printk(KERN_ERR "i2_sitara_init: Error al solicitar memoria para el buffer de transmisión\n");
        iounmap(i2c2_registers);
        return -ENOMEM;
    }

    rx = kmalloc(MAX_SIZE_BUFFER*sizeof(uint32_t), GFP_KERNEL);
    if(rx == NULL)
    {
        printk(KERN_ERR "i2_sitara_init: Error al solicitar memoria para el buffer de recepción\n");
        iounmap(i2c2_registers);
        kfree(trx);
        return -ENOMEM;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_init() OK!\n" );

>>>>>>> tmp
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

    i2c2_registers = NULL;
    
    // Desalojo la memoria de los vectores
    kfree(trx);
    kfree(rx);

    trx_count = 0;
    rx_count = 0;
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
int i2c_sitara_read(const uint8_t slave_address, const uint8_t slave_register, uint8_t *data)
{

    int ret_val = 0;

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
<<<<<<< HEAD

    // Set slave address

    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    // Set data to send

    global_data[0] = slave_register;

    iowrite32(global_data[0], i2c2_registers+I2C_SITARA_DATA);

=======
>>>>>>> tmp
    // pool bus
    ret_val = pool_register(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW, I2C_SITARA_BB, 0, 1000);
    if(ret_val != 0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        return ret_val;
    }
    
    //Loquear el bus
    mutex_lock(&lock_bus);

<<<<<<< HEAD
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
=======
    //Reset FIFOs
    iowrite32(I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR, i2c2_registers+I2C_SITARA_BUF);

    //Configuro el contador
    iowrite32(1, i2c2_registers+I2C_SITARA_CNT);

    // Set slave address
    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    // Set slave register
    //iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);
    trx[0] = slave_register;
    trx_count = 1;
    rx[0] = 0;
    rx_count = 1;

    // Set master reciever mode and start
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_EN | I2C_SITARA_CON_TRX | I2C_SITARA_CON_STT |I2C_SITARA_CON_STP , i2c2_registers+I2C_SITARA_CON);
    
    //Espero la interrupción de xrdy
    if(wait_for_completion_interruptible_timeout(&xrdy, msecs_to_jiffies(1000))==0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        printk(KERN_ERR "i2c_sitara_read: Timeout\n");

        //print raw status
        printk(KERN_INFO "i2c_sitara_read: raw status = 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW));
        return -1;
    }

    //printk(KERN_INFO "i2c_sitara_read: Interrupción de xrdy recibida\n");

    iowrite32(1, i2c2_registers+I2C_SITARA_CNT);
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_EN | I2C_SITARA_CON_STT |I2C_SITARA_CON_STP, i2c2_registers+I2C_SITARA_CON);

    if(wait_for_completion_interruptible_timeout(&rrdy, msecs_to_jiffies(100))==0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        printk(KERN_ERR "i2c_sitara_read: Timeout\n");

        printk(KERN_INFO "i2c_sitara_read: raw status = 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW));

        return -1;
    }
    //printk(KERN_INFO "i2c_sitara_read: Interrupción de rrdy recibida\n");

    /*Read buffer until its empty*/
    *data = rx[rx_count];

    //Liberamos el bus
    mutex_unlock(&lock_bus);
>>>>>>> tmp

    printk(KERN_INFO "i2c_sitara_read: slave_address = 0x%x slave_register = 0x%x data = 0x%x\n", slave_address, slave_register, *data);
   
    return 0;
}

/**
 * @brief Esta función escribe un registro de un esclavo i2c, usando el bus i2c2, del Sitara
 * @param slave_address Dirección del esclavo
 * @param slave_register Dirección del registro a escribir
 * @param data Dato a escribir
 * @return int 
 */
int i2c_sitara_write(const uint8_t slave_address, const uint8_t slave_register, const uint8_t data)
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

    //Loqueamos el buss
    mutex_lock(&lock_bus);

    //Reset FIFOs
    iowrite32(I2C_SITARA_BUF_RXFIFO_CLR | I2C_SITARA_BUF_TXFIFO_CLR, i2c2_registers+I2C_SITARA_BUF);

    //Configuro el contador
    iowrite32(2, i2c2_registers+I2C_SITARA_CNT);

    // Set slave address
    iowrite32(slave_address, i2c2_registers+I2C_SITARA_SA);

    // Set slave register
    //iowrite32(slave_register, i2c2_registers+I2C_SITARA_DATA);

    // Set data
    //iowrite32(data, i2c2_registers+I2C_SITARA_DATA);

    trx[1] = slave_register;
    trx[0] = data;
    trx_count = 2;
    rx[0] = 0;
    rx_count = 1;

    // Set master reciever mode and start
    iowrite32(I2C_SITARA_CON_MST | I2C_SITARA_CON_TRX | I2C_SITARA_CON_EN | I2C_SITARA_CON_STT | I2C_SITARA_CON_STP, i2c2_registers+I2C_SITARA_CON);

    if(wait_for_completion_interruptible_timeout(&xrdy, msecs_to_jiffies(100))==0)
    {
        printk(KERN_ERR "i2c_sitara_read: Error al esperar la interrupción de bus libre\n");
        printk(KERN_ERR "i2c_sitara_read: Timeout\n");

        printk(KERN_INFO "i2c_sitara_read: raw status = 0x%x\n", ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW));

        return -1;
    }

    mutex_unlock(&lock_bus);

    printk(KERN_INFO "i2c_sitara_write: slave_address = 0x%x slave_register = 0x%x data = 0x%x\n", slave_address, slave_register, data);

    return 0;
}

/**
 * @brief 
 * @param slave_address 
 * @return int 
 */
int i2c_sitara_is_connected(uint8_t slave_address)
{
    if(i2c2_registers==NULL)
    {
        printk(KERN_ERR "i2c: Registros no mapeados en memoria, iniciar el bus\n");
        return -1;
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

    //printk(KERN_INFO "i2c_sitara_irq_handler: IRQ\n");

    irq_status_raw = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS_RAW);

    //irq_status = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);

    //printk(KERN_INFO "i2c_sitara_irq_handler: irq_status_raw = %x\n", irq_status_raw);
    
    irq_status = ioread32(i2c2_registers+I2C_SITARA_IRQSTATUS);

    //printk(KERN_INFO "i2c_sitara_irq_handler: irq_status = %x\n", irq_status);

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
<<<<<<< HEAD
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

=======
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_ARDY\n");
            //printk(KERN_INFO "i2c_sitara_irq_handler: Operacion de lectura/escritura finalizada\n");
            //write data
            complete(&ardy);
        }
        if(irq_status_raw & I2C_SITARA_RRDY)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_RRDY\n");
            //printk(KERN_INFO "i2c_sitara_irq_handler: Dato listo para leer\n");
            //read data
            
            if(rx_count == 1)
            {
                rx[rx_count-1] = ioread32(i2c2_registers+I2C_SITARA_DATA);
                rx_count--;
                complete(&rrdy);
            }
            if(rx_count > 1)
            {
                rx[rx_count-1] = ioread32(i2c2_registers+I2C_SITARA_DATA);
                rx_count--;
            }
        }
        if(irq_status_raw & I2C_SITARA_XRDY)
        {
            //write
            if(trx_count == 1)
            {
                iowrite32(trx[trx_count-1], i2c2_registers+I2C_SITARA_DATA);
                trx_count--;
                complete(&xrdy);
            }
            if(trx_count > 1)
            {
                iowrite32(trx[trx_count-1], i2c2_registers+I2C_SITARA_DATA);
                trx_count--;
            }
>>>>>>> tmp
        }
        if(irq_status_raw & I2C_SITARA_GC)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_GC\n");
        }
        if(irq_status_raw & I2C_SITARA_STC)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_STC\n");
        }
        if(irq_status_raw & I2C_SITARA_AERR)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_AERR\n");
        }
        if(irq_status_raw & I2C_SITARA_BF)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_BF\n");
        }
        if(irq_status_raw & I2C_SITARA_AAS)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_AAS\n");
        }
        if(irq_status_raw & I2C_SITARA_XUDF)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XUDF\n");
        }
        if(irq_status_raw & I2C_SITARA_ROVR)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_ROVR\n");
        }
        if(irq_status_raw & I2C_SITARA_BB)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_BB\n");
        }
        if(irq_status_raw & I2C_SITARA_RDR)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_RDR\n");
        }
        if(irq_status_raw & I2C_SITARA_XDR)
        {
            //printk(KERN_INFO "i2c_sitara_irq_handler: I2C_SITARA_IRQSTATUS_XDR\n");
        }
        
        iowrite32(irq_status, i2c2_registers+I2C_SITARA_IRQSTATUS);
    }
    
    //printk(KERN_INFO "i2c_sitara_irq_handler: IRQ handled\n");

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