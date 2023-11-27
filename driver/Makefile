# Nombre: Juan Costa Suárez
# Autor: Juan Costa Suárez
# Fecha: 10/11/2023

EXTRA_CFLAGS := -I$(src)/inc

bmp280_sitara-objs := src/bmp280.o src/i2c_sitara.o	src/bmp280_cdevice.o src/driver.o src/utils.o

obj-m += bmp280_sitara.o

all: clean compile

compile:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rm *.o *.mod.c *.symvers *.order
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
device_tree:
	cd device-tree && make all
install_module:
	sudo insmod bmp280_sitara.ko
	sudo chmod 666 /dev/bmp280_sitara
remove_module:
	sudo rmmod bmp280_sitara
read:
	sudo cat /dev/bmp280_sitara
debug:
	gnome-terminal -- bash -c "sshpass -p temppwd ssh debian@192.168.7.2 'export TERM=xterm-256color; echo temppwd | sudo -S dmesg -wH; bash'" &
get_device_tree:
	sudo cp device-tree/am335x-boneblack.dtb /boot/dtbs/$(shell uname -r)/
connect_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2
copy_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "rm -rf tp_td3"
	sshpass -p temppwd scp -r $(PWD) debian@192.168.7.2:/home/debian/tp_td3
	gnome-terminal -- bash -c "TERM=xterm-256color sshpass -p temppwd ssh debian@192.168.7.2 'cd /home/debian/tp_td3 ; make; echo temppwd; bash'"
open_dmesg_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "dmesg -wH"
