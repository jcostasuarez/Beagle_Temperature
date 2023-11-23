# Nombre: Juan Costa Suárez
# Autor: Juan Costa Suárez
# Fecha: 10/11/2023

EXTRA_CFLAGS := -I$(src)/inc

bmp280_sitara-objs := src/bmp280.o src/i2c_sitara.o

obj-m += bmp280_sitara.o

all: clean compile 

compile:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

device_tree:
	cd device-tree && make all

install_module:
	sudo rmmod bmp280_i2c
	sudo rmmod bmp280_spi
	sudo rmmod bmp280
	sudo insmod bmp280_sitara.ko

remove_module:
	sudo rmmod bmp280_sitara

read:
	sudo cat /dev/temp_sensor/bmp280

debug_new_window:
	gnome-terminal -- bash -c ""; exec bash"
	gnome-terminal -- bash -c "tail -f /var/log/kern.log"

get_device_tree:
	sudo cp device-tree/am335x-boneblack.dtb /boot/dtbs/$(shell uname -r)/

connect_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2
copy_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "rm -rf tp_td3"
	sshpass -p temppwd scp -r $(PWD) debian@192.168.7.2:/home/debian/tp_td3
	sshpass -p temppwd ssh debian@192.168.7.2

