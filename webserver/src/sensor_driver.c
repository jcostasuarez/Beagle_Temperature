#include "../inc/sensor_driver.h"

float get_temp(void)
{
    int fd;
    char buf[2];
    float temp;

    if ((fd = open(I2C_BUS_PATH, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        exit(1);
    }

    if (ioctl(fd, I2C_SLAVE, BMP280_ADDR) < 0) {
        perror("Failed to select the device");
        exit(1);
    }

    // Read temperature data
    buf[0] = TEMP_DATA_REG;
    if (write(fd, buf, 1) != 1) {
        perror("Failed to write to the i2c bus");
        exit(1);
    }

    if (read(fd, buf, 2) != 2) {
        perror("Failed to read from the i2c bus");
        exit(1);
    }

    temp = (buf[0] << 8 | buf[1]) / 256.0;

    close(fd);

    return temp;
}
// Función para inicializar el sensor BMP280
size_t init_sensor(void)
{
    // File descriptor del bus I2C
    int fd;

    // Buffer para datos a escribir en el bus I2C
    char buf[2];


    if ((fd = open(I2C_BUS_PATH, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        exit(1);
    }

    if (ioctl(fd, I2C_SLAVE, BMP280_ADDR) < 0) {
        perror("Failed to select the device");
        exit(1);
    }

    // Set temperature oversampling to 1
    buf[0] = TEMP_OVERSAMPLING_REG;
    buf[1] = TEMP_OVERSAMPLING_SETTING;
    if (write(fd, buf, 2) != 2) {
        perror("Failed to write to the i2c bus");
        exit(1);
    }

    close(fd);

    return 0;
}


