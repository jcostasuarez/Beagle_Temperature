/**
 * @file bmp280.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief Driver para el sensor de temperatura BMP280
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/* Includes */

#include "bmp280.h"
#include "i2c_sitara.h"

/* Defines */

#define MINOR_NUMBER 0
#define NUMBER_OF_DEVICES 1
#define DEVICE_CLASS_NAME "temp"
#define DEVICE_NAME "bmp280_sitara"

/* Static variables */

static struct class *device_class;
static dev_t device_number;
static struct cdev *char_device;

static uint32_t dig_T1;
static int32_t dig_T2;
static int32_t dig_T3;

static volatile int bmp_working = False;

/* Private functions prototypes */

/* File operations */

static int char_bmp280_open(struct inode *inode, struct file *file);
static int char_bmp280_close(struct inode *inode, struct file *file);
static ssize_t char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);

/* Driver functions prototypes */
static int driver_bmp280_init(void);
static void driver_bmp280_exit(void);
static int driver_bmp280_probe( struct platform_device *pdev );
static int driver_bmp280_remove( struct platform_device *pdev );


/*Funciónes del módulo*/
static int bmp280_init(void);
static void bmp280_deinit(void);
static int bmp280_is_connected(void);
static int bmp280_set_frequency(bmp280_freq_t frequency);
static int bmp280_set_mode(bmp280_mode_t mode);
static int bmp280_get_temperature(bmp280_temperature *temperature);
static int driver_is_opened(void );

/* Static variables*/

/****************DRIVER****************/

static struct of_device_id bmp280_of_match[] = 
{
    {
        .compatible = DEVICE_NAME,
    },
    {},
};

static struct platform_driver bmp280_driver = 
{
    .probe = driver_bmp280_probe,
    .remove = driver_bmp280_remove,
    .driver = 
    {
        .name = DEVICE_NAME,
        .of_match_table = of_match_ptr(bmp280_of_match),
    },
};

/* File operations */

static const struct file_operations bmp280_fops =
{
    .owner = THIS_MODULE,
    .open = char_bmp280_open,
    .release = char_bmp280_close,
    .read = char_bmp280_read,
    .write = char_bmp280_write,
    /*.unlocked_ioctl = bmp280_ioctl*/
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);
module_init(driver_bmp280_init);
module_exit(driver_bmp280_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Juan Costa Suárez");
MODULE_DESCRIPTION("Driver para el sensor bmp280, usando i2c2, y Beaglebone Black");
MODULE_VERSION("1.0");

/* Functions */

/// @brief Creates the char device
/// @param 
/// @return "0"on success, non zero error code on error.
int char_device_create_bmp280(void)
{
    int retval = -1;

    printk(KERN_INFO "Creando el char device\n");

    //cdev_alloc

    char_device = cdev_alloc();

    if (char_device == NULL)
    {
        pr_err("[INIT] ERR: TD3_I2C fallo cdev_alloc() (%s %d)\n", __FUNCTION__, __LINE__);
        return -1;
    }

    printk(KERN_INFO "cdev_alloc() OK!\n");

    //alloc_chrdev_region

    if(alloc_chrdev_region(&device_number, MINOR_NUMBER, NUMBER_OF_DEVICES, DEVICE_NAME) < 0)
    {
        printk(KERN_ERR "Error al reservar el device number\n");
        retval = -1;
        goto kmalloc_error;
    }

    printk(KERN_INFO "Device number reservado correctamente\n");

    cdev_init(char_device, &bmp280_fops);
    
    printk(KERN_INFO "cdev_init() OK!\n");

    if(cdev_add(char_device, device_number, NUMBER_OF_DEVICES) < 0)
    {
        printk(KERN_ERR "Error al agregar el device\n");
        retval = -1;
        goto device_error;
    }

    if((device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME)) == NULL)
    {
        printk(KERN_ERR "Error al crear la clase del device\n");
        retval = -1;
        goto chrdev_error;
    }

    printk(KERN_INFO "Device class creado correctamente\n");

    if(device_create(device_class, NULL, device_number, NULL, DEVICE_NAME) == NULL)
    {
        printk(KERN_ERR "Error al crear el device\n");
        retval = -1;
        goto class_error;
    }

    printk(KERN_INFO "Char device creado correctamente\n");

    return 0;

    device_error: device_destroy(device_class, device_number);
    class_error: class_destroy(device_class);
    chrdev_error: unregister_chrdev(device_number, DEVICE_NAME);
    kmalloc_error: kfree(DEVICE_NAME);
    return retval;
}   

/// @brief Removes the char device
/// @param void
/// @return void
void char_device_remove(void)
{
    pr_info("[LOG] Removing char device.\n");

    cdev_del(char_device);
    device_destroy(device_class, device_number);
    class_destroy(device_class);
    unregister_chrdev(device_number, DEVICE_NAME);
    kfree(DEVICE_NAME);
}

/* Private functions */

/*********CHAR DEVICE**********/

static int char_bmp280_open(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Abriendo el archivo\n");

    if(driver_is_opened() == False)
    {
        printk("El driver no está abierto\n");
        return -1;
    }

    if(!bmp280_is_connected()) 
    {
        printk("Couldn't open device.\n");
        return -1;
    }

    return 0;
}

static int char_bmp280_close(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Cerrando el archivo\n");


    return 0;
}

static int char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    bmp280_temperature temperature;

    pr_info("[LOG] Leyendo el archivo\n");

    if(bmp280_get_temperature(&temperature) < 0)
    {
        pr_err("[LOG] Error al obtener la temperatura\n");

        return -1;
    }

    if(copy_to_user(buf, &temperature, sizeof(bmp280_temperature)) != 0)
    {
        pr_err("[LOG] Error al copiar la temperatura al usuario\n");

        return -1;
    }

    return 0;
}

static int char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    pr_info("[LOG] Escribiendo el archivo\n");

    return 0;
}

static int __init driver_bmp280_init(void)
{
    int retval = 0;

    printk(KERN_INFO "Inicializando el driver\n");

    if((retval = char_device_create_bmp280()) < 0)
    {
        printk(KERN_ERR "Error al crear el char device\n");
        goto char_device_error;
    }

    if((retval = platform_driver_register(&bmp280_driver)) < 0)
    {
        printk(KERN_INFO "Error al registrar el driver\n");
        goto platform_driver_error;
    }
    
    printk(KERN_INFO "Driver inicializado correctamente\n");

    return 0;

    platform_driver_error: char_device_remove();
    char_device_error: return -1;

}

static void __exit driver_bmp280_exit(void)
{
    printk(KERN_INFO "Saliendo del driver\n");

    char_device_remove();

    platform_driver_unregister(&bmp280_driver);

    printk(KERN_INFO "Plataform driver unregistered\n");
}

/* Device tree functions */

static int driver_bmp280_probe( struct platform_device *pdev )
{

    int retval = -1;

    printk(KERN_INFO "Inicializando probe.\n");

    if((retval = i2c_sitara_init()) < 0)
    {
        printk(KERN_ERR "Error al inicializar el i2c\n");
        goto i2c_sitara_error;
    }

    if((retval = bmp280_init()) < 0)
    {
        printk(KERN_ERR "Error al inicializar el bmp280\n");
        goto bmp280_error;
    }

    printk(KERN_INFO "Probe completado\n");

    bmp_working = True;

    return 0;

    bmp280_error: i2c_sitara_exit();
    i2c_sitara_error: return -1;
}

static int driver_bmp280_remove( struct platform_device *pdev )
{
    printk(KERN_INFO "Saliendo del probe\n");

    char_device_remove();
    i2c_sitara_exit();
    bmp280_deinit();

    return 0;
}

/* Static Functions */

static int bmp280_init(void)
{
    uint8_t aux1 = 0;
    uint8_t aux2 = 0;

    if(bmp280_is_connected() < 0)
    {
        pr_err("[LOG] Error: El chip no está conectado\n");

        return -1;
    }

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_LSB, NOMASK, &aux2);

    dig_T1 = (aux1 << 8) | aux2;

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, NOMASK, &aux2);

    dig_T2 = (aux1 << 8) | aux2;

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, NOMASK, &aux2);

    dig_T3 = (aux1 << 8) | aux2;

    bmp280_set_frequency(FREQ_1);
    bmp280_set_mode(BMP280_NORMAL_MODE);

    pr_info("[LOG] BMP280 configurado correctamente\n");

    return 0;

}

static void bmp280_deinit(void)
{
    bmp280_set_mode(BMP280_SLEEP_MODE);
}

static int bmp280_is_connected(void)
{
    uint8_t data;

    if(i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_ID,  NOMASK, &data) != BMP280_CHIP_ID)
    {
        pr_err("[LOG] Error: El chip no está conectado\n");
        return -1;
    }

    return 0;
}

static int bmp280_set_frequency(bmp280_freq_t frequency)
{
    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CONFIG, (uint8_t)frequency) < 0)
    {
        pr_err("[LOG] Error: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

static int bmp280_set_mode(bmp280_mode_t mode)
{
    if(i2c_sitara_write(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_CTRL_MEAS, (uint8_t)mode) < 0)
    {
        pr_err("[LOG] Error: No se pudo escribir en el registro\n");
        return -1;
    }

    return 0;
}

static int driver_is_opened(void )
{
    return bmp_working;
}

static int bmp280_get_temperature(bmp280_temperature *temperature)
{
    int32_t data[3];
    int32_t raw_temp;
    int32_t var1, var2, t_fine;
    uint8_t aux1 = 0;
    uint8_t aux2 = 0;

    if(temperature == NULL)
    {
        printk("[LOG] Error: temperature is NULL\n");

        return -1;
    }

    if(i2c_sitara_is_connected(BMP280_SLAVE_ADDRESS) < 0)
    {
        printk("[LOG] Error: El i2c no está conectado\n");

        return -1;
    }

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T1_COMP_LSB, NOMASK, &aux2);

    data[0]  = (int32_t)((aux1 << 8) | aux2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T2_COMP_LSB, NOMASK, &aux2);

    data[1]  = (int32_t)((aux1 << 8) | aux2);

    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_MSB, NOMASK, &aux1);
    i2c_sitara_read(BMP280_SLAVE_ADDRESS, BMP280_ADRESS_T3_COMP_LSB, NOMASK, &aux2);

    data[2]  = (int32_t)((aux1 << 8) | aux2);

    if(data[0] < 0 || data[1] < 0 || data[2] < 0)
    {
        printk("[LOG] Error: i2c_sitara_read\n");

        return -1;
    }

    raw_temp = (data[0] << 16) | (data[1] << 8) | (data[2] >> 4);
    
    var1 = ((((raw_temp >> 3) - (dig_T1 << 1))) * (dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - (dig_T1)) * ((raw_temp >> 4) - (dig_T1))) >> 12) * (dig_T3)) >> 14;

    t_fine = var1 + var2;

    *temperature = (t_fine * 5 + 128) >> 8;

    return 0;
}