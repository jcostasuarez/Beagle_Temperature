/* File operations */
#include "bmp280_cdevice.h"
#include "bmp280.h"

static int char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static int char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static int char_bmp280_open(struct inode *inode, struct file *file);
static int char_bmp280_close(struct inode *inode, struct file *file);

static struct class *device_class = NULL;
static dev_t device_number;
static struct cdev char_device;
static char *device_name = NULL;

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

/*********CHAR DEVICE**********/

/// @brief Creates the char device
/// @param 
/// @return "0"on success, non zero error code on error.
int char_device_create_bmp280(void)
{
    int ret_val = -1;

    printk(KERN_INFO "Inicializando el char device\n");

    if ((device_name = kmalloc(strlen(DEVICE_NAME) + 1, GFP_KERNEL)) == NULL) 
    {
        printk(KERN_ERR "Fallo kmalloc\n");
        return -ENOMEM;
    }
    strcpy(device_name, DEVICE_NAME);

    printk(KERN_INFO "Device name: %s\n", device_name);

    if(alloc_chrdev_region(&device_number, MINOR_NUMBER, NUMBER_OF_DEVICES, device_name) != 0)
    {
        printk(KERN_ERR "Error al reservar el device number\n");
        return -EBUSY;
    }

    printk(KERN_INFO "Device number reservado correctamente\n");

    if((device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME))==NULL)
    {
        printk(KERN_ERR "Error al crear la clase del device\n");
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "Device class creado correctamente\n");

    if(device_create(device_class, NULL, device_number, NULL, device_name)==NULL)
    {
        printk(KERN_ERR "Error al crear el device\n");
        class_destroy(device_class);
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "Device creado correctamente\n");

    cdev_init(&char_device, &bmp280_fops);

    printk(KERN_INFO "cdev_init() OK!\n");

    if ((ret_val = cdev_add(&char_device, device_number, NUMBER_OF_DEVICES)) != 0 ) 
    {
        printk(KERN_ERR "Error al agregar el device\n");
        class_destroy(device_class);
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "Device creado correctamente\n"
                    "Major number: %d\n"
                    "Minor number: %d\n", MAJOR(device_number), MINOR(device_number));
                    
    return 0;

}   

/// @brief Removes the char device
/// @param void
/// @return void
void char_device_remove(void)
{
    pr_info("[LOG] Removing char device.\n");

    cdev_del(&char_device); 

    printk(KERN_INFO "char_device_remove: cdev_del() OK!\n");

    device_destroy(device_class, device_number);
    
    printk(KERN_INFO "char_device_remove: device_destroy() OK!\n");

    class_destroy(device_class);

    printk(KERN_INFO "char_device_remove: class_destroy() OK!\n");

    unregister_chrdev(device_number, device_name);

    printk(KERN_INFO "char_device_remove: unregister_chrdev() OK!\n");

    kfree(device_name);

    printk(KERN_INFO "char_device_remove: kfree() OK!\n");

    pr_info("[LOG] Char device removed.\n");
}

/* File operations */

static int char_bmp280_open(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Abriendo el archivo\n");

    if(bmp_is_running()== False)
    {
        printk("El driver no está abierto\n");
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
/* Device tree functions */