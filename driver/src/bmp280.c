#include "bmp280.h"


/* Global variables*/


/*********CHAR DEVICE**********/

/* Static functions*/

static int bmp280_open(struct inode *inode, struct file *file)
{
    unsigned int maj = imajor(inode);
    unsigned int min = iminor(inode);

    pr_info("[LOG] Major: %d, Minor: %d\n", maj, min);

    struct bmp280_dev *bmp280_devp;

    bmp280_devp = container_of(inode->i_cdev, struct bmp280_dev, cdev);
    
    if(bmp280_devp == NULL)
    {
        pr_err("[LOG] Error al obtener el puntero al dispositivo\n");
        return -1;
    }

    file->private_data = bmp280_devp;

    return 0;
}

static int bmp280_close(struct inode *inode, struct file *file)
{
    pr_info("[LOG] Cerrando el archivo\n");

    struct bmp280_dev *bmp280_devp = NULL;
    
    return 0;
}

static ssize_t bmp280_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    return 0;
}

static ssize_t bmp280_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    return 0;
}

/* Static variables*/

static const struct file file_operations bmp280_fops =
{
    .owner = THIS_MODULE,
    .open = bmp280_open,
    .release = bmp280_close,
    .read = bmp280_read,
    .write = bmp280_write,
    /*.unlocked_ioctl = bmp280_ioctl*/
};

/****************DRIVERS****************/

static struct
{
  dev_t TD3_I2C_DEV;
  struct cdev *TD3_I2C_CHAR_DEV;
  struct device *TD3_I2C_DEVICE;
  struct class *TD3_I2C_DEV_CLASS;
} state;

static struct file_operations TD3_FOPS =
{
    .owner = THIS_MODULE,
    .open = td3_i2c_open,
    .release = td3_i2c_close,
    .read = td3_i2c_read,
    .write = td3_i2c_write,
    .unlocked_ioctl = td3_i2c_ioctl
};

static struct of_device_id i2c_of_device_ids[] =
{
    {
        .compatible = NAME,
    },
{}};

MODULE_DEVICE_TABLE(of, i2c_of_device_ids);

static struct platform_driver TD3_PLAT_DRIVER =
{
    .probe = bmp_280_probe,
    .remove = bmp_280_remove,
    .driver =
        {
            .name = NAME,
            .of_match_table = of_match_ptr(i2c_of_device_ids),
        },
};

static void bmp280_init(void)
{
    int ret_val = 0;

    if(ret_val = bmp280_config(void) != 0)
    {
        return ret_val;
    }

    if(ret_val = bmp280_reset_regs(void) != 0)
    {
        return ret_val;
    }

    buffer[0] = MPU6050_RA_PWR_MGMT_1; //Salgo sleep mode y habilito los sensores
    buffer[1] = 0x00;
    I2C_Write_n_Bytes(buffer, 2);

    msleep(100);

    buffer[0] = MPU6050_RA_PWR_MGMT_1; // Seteo la fuente de clock com pll del giroscopo
    buffer[1] = 0x01;
    I2C_Write_n_Bytes(buffer, 2);

    // Configure Gyro and Accelerometer

    //Configuro gyro y acacelerometro 1khz
    buffer[0] = MPU6050_RA_CONFIG;
    buffer[1] = 0x03;
    I2C_Write_n_Bytes(buffer, 2);

    // Seteo el tiempo de muestra gyroscope output rate/(1 + SMPLRT_DIV)
    buffer[0] = MPU6050_RA_SMPLRT_DIV; // 200Hz
    buffer[1] = 0x04;
    I2C_Write_n_Bytes(buffer, 2);

    // Seteo el gyroscopo a escala maxima
    buffer[0] = MPU6050_RA_GYRO_CONFIG;
    I2C_Write_n_Bytes(buffer, 1);

    I2C_Read_n_Bytes(buffer, 1);
    aux = buffer[0];
    buffer[0] = MPU6050_RA_GYRO_CONFIG; // limpio self-test bits [7:5]
    buffer[1] = (aux & ~0xE0);
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_GYRO_CONFIG; // limpio AFS bits [4:3]
    buffer[1] = (aux & ~0x18);
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_GYRO_CONFIG;
    buffer[1] = (aux | Gscale << 3); // Seteo a maxima escala
    I2C_Write_n_Bytes(buffer, 2);

    // Seteo el acelerometro
    buffer[0] = MPU6050_RA_ACCEL_CONFIG;
    I2C_Write_n_Bytes(buffer, 1);

    I2C_Read_n_Bytes(buffer, 1);
    aux = buffer[0];
    buffer[0] = MPU6050_RA_ACCEL_CONFIG;
    buffer[1] = (aux & ~0xE0); // limpio self-test bits [7:5]
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_ACCEL_CONFIG;
    buffer[1] = (aux & ~0x18); // limpio AFS bits [4:3]
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_ACCEL_CONFIG;
    buffer[1] = (aux | Ascale << 3); // Seteo a maxima escala el acelerometro
    I2C_Write_n_Bytes(buffer, 2);

    // Interrupciones y bypass
    buffer[0] = MPU6050_RA_INT_PIN_CFG;
    buffer[1] = 0x02;
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_INT_ENABLE;
    buffer[1] = 0x01;
    I2C_Write_n_Bytes(buffer, 2);

    // Habilito FIFO
    buffer[0] = MPU6050_RA_USER_CTRL;
    I2C_Write_n_Bytes(buffer, 1);

    I2C_Read_n_Bytes(buffer, 1);
    aux = buffer[0];
    buffer[0] = MPU6050_RA_USER_CTRL;
    buffer[1] = (aux | 0x44);
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_USER_CTRL;
    I2C_Write_n_Bytes(buffer, 1);
    I2C_Read_n_Bytes(buffer, 1);
    aux = buffer[0];

    pr_info("[LOG USER CONTROL]0x%x\n", aux);

    // Habilito los sensores hacia la FIFO
    buffer[0] = MPU6050_RA_FIFO_EN;
    buffer[1] = 0xF8;
    I2C_Write_n_Bytes(buffer, 2);

    buffer[0] = MPU6050_RA_FIFO_EN;
    I2C_Write_n_Bytes(buffer, 1);
    I2C_Read_n_Bytes(buffer, 1);
    aux = buffer[0];

    pr_info("[LOG RA FIFO EN]0x%x\n", aux);
}

/**
 * @brief Funcion de lectura de los registros de data de la fifo
 * 
 * @return uint16_t 
 */
static uint16_t MPU6050_Read_Data_Count_Fifo(void)
{
    uint16_t contador;
    uint8_t *buffer_tx;

    buffer_tx = kmalloc(sizeof(uint8_t), GFP_KERNEL);

    if (buffer_tx == NULL)
    {
        return -1;
    }

    *buffer_tx = MPU6050_RA_FIFO_COUNTH;
    I2C_Write_n_Bytes(buffer_tx, 1);
    I2C_Read_n_Bytes(buffer_tx, 1);
    pr_info("[LOG Count H]0x%x\n", *buffer_tx);

    contador = *buffer_tx;

    *buffer_tx = MPU6050_RA_FIFO_COUNTL;
    I2C_Write_n_Bytes(buffer_tx, 1);
    I2C_Read_n_Bytes(buffer_tx, 1);
    pr_info("[LOG Count L]0x%x\n", *buffer_tx);

    contador = contador << 8 | *buffer_tx;

    kfree(buffer_tx);

    return contador;
}

// 
// t_fine carries fine temperature as global value

/// @brief Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
/// @param adc_T 
/// @return 
bmp280_temperature bmp280_compensate_t(bmp280_temperature adc_T)
{
    bmp280_temperature var1, var2, T;
    var1 = ((((adc_T>>3) – ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
    var2 = (((((adc_T>>4) – ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) – ((BMP280_S32_t)dig_T1))) >> 12) *
    ((BMP280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}