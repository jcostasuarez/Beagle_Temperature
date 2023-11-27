/* File operations */
#include "bmp280_cdevice.h"
#include "bmp280.h"
#include "i2c_sitara.h"

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

    printk(KERN_INFO "char_device_create_bmp280: Creando el char device\n");

    if ((device_name = kmalloc(strlen(DEVICE_NAME) + 1, GFP_KERNEL)) == NULL) 
    {
        printk(KERN_ERR "char_device_create_bmp280: Error al reservar memoria para el nombre del device\n");
        return -ENOMEM;
    }
    strcpy(device_name, DEVICE_NAME);

    printk(KERN_INFO "char_device_create_bmp280: device_name = %s\n", device_name);

    if(alloc_chrdev_region(&device_number, MINOR_NUMBER, NUMBER_OF_DEVICES, device_name) != 0)
    {
        printk(KERN_ERR "char_device_create_bmp280: Error al reservar el device number\n");
        return -EBUSY;
    }

    printk(KERN_INFO "char_device_create_bmp280: device_number = %d\n", device_number);

    if((device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME))==NULL)
    {
        printk(KERN_ERR "char_device_create_bmp280: Error al crear el device class\n");
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "char_device_create_bmp280: device_class creado correctamente\n");

    if(device_create(device_class, NULL, device_number, NULL, device_name)==NULL)
    {
        printk(KERN_ERR "char_device_create_bmp280: Error al crear el device\n");
        class_destroy(device_class);
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "char_device_create_bmp280: device creado correctamente\n");

    cdev_init(&char_device, &bmp280_fops);

    printk(KERN_INFO "char_device_create_bmp280: cdev_init() OK!\n");

    if ((ret_val = cdev_add(&char_device, device_number, NUMBER_OF_DEVICES)) != 0 ) 
    {
        printk(KERN_ERR "char_device_create_bmp280: Error al agregar el char device\n");
        class_destroy(device_class);
        unregister_chrdev_region(device_number, NUMBER_OF_DEVICES);
        return -EBUSY;
    }

    printk(KERN_INFO "char_device_create_bmp280: cdev_add() OK!\n");

    printk(KERN_INFO "char_device_create_bmp280: Char device creado correctamente\n");
    printk(KERN_INFO "char_device_create_bmp280: Major number = %d\n", MAJOR(device_number));
    printk(KERN_INFO "char_device_create_bmp280: Minor number = %d\n", MINOR(device_number));   
                    
    return 0;

}   

/// @brief Removes the char device
/// @param void
/// @return void
void char_device_remove(void)
{
    printk(KERN_INFO "char_device_remove: Removiendo el char device\n");

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

    printk(KERN_INFO "char_device_remove: Char device removido correctamente\n");
}

/* File operations */

static int char_bmp280_open(struct inode *inode, struct file *file)
{
    int retval = -1;

    printk(KERN_INFO "char_bmp280_open: Abriendo el archivo\n");

    if((retval = bmp280_init()) < 0)
    {
        printk(KERN_ERR "char_bmp280_open: Error al inicializar el bmp280\n");
        return -1;
    }
    printk(KERN_INFO "char_bmp280_open: bmp280_init() OK!\n");

    return 0;
}

static int char_bmp280_close(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "char_bmp280_close: Cerrando el archivo\n");

    bmp280_deinit();

    printk(KERN_INFO "char_bmp280_close: Archivo cerrado\n");

    return 0;
}

static int char_bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{

    int temperatura;
    char string_temperatura[10];
    int string_temperatura_len = 0;

    printk(KERN_INFO "char_bmp280_read: Leyendo el archivo\n");

    if((bmp280_get_temperature(&temperatura)) != 0)
    {
        printk(KERN_ERR "char_bmp280_read: Error al obtener la temperatura\n");

        return -1;
    }

    sprintf(string_temperatura, "%i\n", temperatura);

    string_temperatura_len = strlen(string_temperatura);

    if(copy_to_user(buf, string_temperatura, string_temperatura_len) != 0)
    {
        printk(KERN_ERR "char_bmp280_read: Error al copiar la temperatura al usuario\n");

        return -1;
    }

    // Change the offset

    *offset += string_temperatura_len;

    printk(KERN_INFO "char_bmp280_read: Temperatura = %s\n", string_temperatura);
    printk(KERN_INFO "char_bmp280_read: Temperatura copiada al usuario\n");

    return string_temperatura_len;
}

static int char_bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    printk(KERN_INFO "char_bmp280_write: Escribiendo en el archivo\n");

    printk(KERN_INFO "char_bmp280_write: Operación no realizable\n");

    return 0;
}