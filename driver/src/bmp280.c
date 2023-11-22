/* Includes */

#include "bmp280.h"
#include "i2c_sitara.h"

/* Defines */

#define MINOR_NUMBER 0
#define NUMBER_OF_DEVICES 1
#define DEVICE_CLASS_NAME "temp_sensor"
#define DEVICE_NAME "bmp280"

/* Static variables */

static char* device_name = NULL;
static dev_t device_number;
static struct class *device_class;
static struct cdev my_device;
static uint32_t dig_T1;
static int32_t dig_T2;
static int32_t dig_T3;

/* Private functions prototypes */

static int char_bmp280_open(struct inode *inode, struct file *file);
static int char_bmp280_close(struct inode *inode, struct file *file);
static ssize_t char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static int driver_bmp280_probe( struct platform_device *pdev );
static int driver_bmp280_remove( struct platform_device *pdev );
static int bmp280_init(void);
static void bmp280_deinit(void);
static int bmp280_is_connected(void);
static int bmp280_set_frequency(bmp280_freq_t frequency);
static int bmp280_set_mode(bmp280_mode_t mode);
static int bmp280_get_temperature(bmp280_temperature *temperature);

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


/* Functions */

/// @brief Creates the char device
/// @param 
/// @return "0"on success, non zero error code on error.
int char_device_create_bmp280(void)
{
    int retval = -1;

    // Get own copy of name
    if((device_name = kmalloc(strlen(DEVICE_NAME) + 1, GFP_KERNEL)) == NULL)
    {
        pr_err("[LOG] Out of memory for char device.\n");
        goto name_error;
    }

    strcpy(device_name, DEVICE_NAME);

    if((retval = alloc_chrdev_region(&device_number, MINOR_NUMBER, NUMBER_OF_DEVICES, device_name)) != 0)
    {
        pr_err("[LOG] Couldn't allocate device number.\n");
        goto kmalloc_error;
    }

    if((device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME)) == NULL)
    {
        pr_err("[LOG] Device class couldn't be created.\n");
        retval = -1;
        goto chrdev_error;
    }

    if(device_create(device_class, NULL, device_number, NULL, device_name) == NULL)
    {
        pr_err("[LOG] Device couldn't be created.\n");
        retval = -1;
        goto class_error;
    }

    cdev_init(&my_device, &bmp280_fops);

    if(cdev_add(&my_device, device_number, NUMBER_OF_DEVICES) < 0)
    {
        pr_err("[LOG] Couldn't add the device to the system.\n");
        retval = -1;
        goto device_error;
    }

    pr_info("[LOG] Char device created successfully.\n");

    return 0;

    device_error: device_destroy(device_class, device_number);
    class_error: class_destroy(device_class);
    chrdev_error: unregister_chrdev(device_number, device_name);
    kmalloc_error: kfree(device_name);
    name_error: return retval;
}   

/// @brief Removes the char device
/// @param void
/// @return void
void char_device_remove(void)
{
    pr_info("[LOG] Removing char device.\n");

    cdev_del(&my_device);
    device_destroy(device_class, device_number);
    class_destroy(device_class);
    unregister_chrdev(device_number, device_name);
    kfree(device_name);
}

/* Private functions */

/*********CHAR DEVICE**********/

static int char_bmp280_open(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Abriendo el archivo\n");

    if(!bmp280_is_connected()) 
    {
        printk("Couldn't open device.\n");
        return -1;
    }

    return 0;
}

int char_bmp280_close(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Cerrando el archivo\n");


    return 0;
}

ssize_t char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
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

ssize_t char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    pr_info("[LOG] Escribiendo el archivo\n");

    return 0;
}

/* Static variables*/

/****************DRIVER****************/

static struct of_device_id bmp280_of_match[] = 
{
    {
        .compatible = "bosch,bmp280",
    },
    {},
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);

static struct platform_driver bmp280_driver = 
{
    .probe = driver_bmp280_probe,
    .remove = driver_bmp280_remove,
    .driver = 
    {
        .name = "bmp280",
        .of_match_table = bmp280_of_match,
    },
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);

static int __init driver_bmp280_init(void)
{
    int retval = -1;

    pr_info("[LOG] Inicializando el driver\n");

    if((retval = platform_driver_register(&bmp280_driver)) < 0)
    {
        pr_err("[LOG] Error al registrar el driver\n");
        goto platform_driver_error;
    }

    
    pr_info("[LOG] Driver inicializado correctamente\n");

    return 0;

    platform_driver_error: return retval;
}

static void __exit driver_bmp280_exit(void)
{
    pr_info("[LOG] Saliendo del driver\n");

    platform_driver_unregister(&bmp280_driver);
}

module_init(driver_bmp280_init);
module_exit(driver_bmp280_exit);

/* Device tree functions */

static int driver_bmp280_probe( struct platform_device *pdev )
{
    int retval = -1;

    pr_info("[LOG] Inicializando el driver\n");

    if((retval = i2c_sitara_init()) < 0)
    {
        pr_err("[LOG] Error al inicializar el i2c\n");
        goto i2c_sitara_error;
    }

    if((retval = bmp280_init()) < 0)
    {
        pr_err("[LOG] Error al inicializar el bmp280\n");
        goto bmp280_error;
    }

    if((retval = char_device_create_bmp280()) < 0)
    {
        pr_err("[LOG] Error al crear el char device\n");
        goto char_device_error;
    }

    pr_info("[LOG] Driver inicializado correctamente\n");

    return 0;

    char_device_error: char_device_remove();
    bmp280_error: bmp280_deinit();
    i2c_sitara_error: return retval;
}

static int driver_bmp280_remove( struct platform_device *pdev )
{
    pr_info("[LOG] Saliendo del driver\n");

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