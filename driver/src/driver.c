#include "bmp280.h"
#include "bmp280_cdevice.h"
#include "i2c_sitara.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Juan Costa Suárez");
MODULE_DESCRIPTION("Driver para el sensor bmp280, usando i2c2, y Beaglebone Black");
MODULE_VERSION("1.0");

/* Private functions prototypes */

static int driver_bmp280_remove( struct platform_device *pdev );
static int driver_bmp280_probe( struct platform_device *pdev );

/****************DRIVER****************/

static struct of_device_id bmp280_of_match[] = 
{
    {
        .compatible = DEVICE_NAME,
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
        .name = DEVICE_NAME,
        .of_match_table = of_match_ptr(bmp280_of_match),
        .owner = THIS_MODULE,
    },
};

/* funciones*/

static int __init driver_bmp280_init(void)
{
    int ret_val = 0;

    /**Mensaje de inicio*/
    printk(KERN_INFO "\n\n\n\n\n    driver_bmp280_init: Inicializando el driver    \n\n\n\n\n");

    if((ret_val = platform_driver_register(&bmp280_driver)) < 0)
    {
        printk(KERN_INFO "Error al registrar el driver\n");
        return -1;
    }
    
    printk(KERN_INFO "driver_bmp280_init: Driver inicializado correctamente\n");

    return 0;
}

static void __exit driver_bmp280_exit(void)
{
    printk(KERN_INFO "\n\n\n\n\n    driver_bmp280_exit: Desinstalando el driver    \n\n\n\n\n");

    platform_driver_unregister(&bmp280_driver);

    printk(KERN_INFO "driver_bmp280_exit: Driver desinstalado correctamente\n");
}

module_init(driver_bmp280_init);
module_exit(driver_bmp280_exit);


static int driver_bmp280_probe( struct platform_device *pdev )
{
    int retval = -1;

    printk(KERN_INFO "\n\n\n\n\n    driver_bmp280_probe: Probe del driver    \n\n\n\n\n");


    if((retval = char_device_create_bmp280()) < 0)
    {
        printk(KERN_ERR "driver_bmp280_probe: Error al crear el char device\n");
        return -1;
    }

    /*Configuro los pines del I2C2*/
    
    if((retval = i2c_sitara_config_pinmux()) != 0)
    {
        printk( KERN_ERR "Error al configurar los pines del I2C2\n");
        char_device_remove();
        return retval;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_pinmux() OK!\n" );


    if((retval = i2c_sitara_turn_on_peripheral()) != 0)
    {
        printk( KERN_ERR "Error al configurar el periferico del I2C2\n");
        char_device_remove();
        return retval;
    }
    
    /*Configuro las interrupciones del I2C2*/

    if((retval = i2c_sitara_config_interrupts(pdev)) != 0)
    {
        printk( KERN_ERR "Error al configurar las interrupciones del I2C2\n");
        char_device_remove();
        return retval;
    }

    // Inicio i2c

    if(i2c_sitara_init() != 0)
    {
        printk( KERN_ERR "Error al inicializar el I2C2\n");
        i2c_sitara_free_interrupts();
        char_device_remove();
        return -1;
    }

    printk(KERN_INFO "i2_sitara_init: i2c_sitara_config_interrupts() OK!\n" );

    printk(KERN_INFO "driver_bmp280_probe: char_device_create_bmp280() OK!\n");

    printk(KERN_INFO "\n\n\n\n\n    driver_bmp280_probe: Probe del driver finalizado      \n\n\n\n\n");

    return 0;
}

static int driver_bmp280_remove( struct platform_device *pdev )
{
    printk(KERN_INFO "driver_bmp280_remove: Removiendo el driver bmp280\n");

    i2c_sitara_exit();

    i2c_sitara_free_interrupts();

    char_device_remove();    
    
    printk(KERN_INFO "driver_bmp280_remove: Driver removido correctamente\n");

    return 0;
}
