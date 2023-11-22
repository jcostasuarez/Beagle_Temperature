obj-m += bmp280.o
EXTRA_CFLAGS := -I$(src)/inc

bmp280-objs := src/bmp280.o src/i2c_sitara.o

all: compile device_tree

compile:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules >> log.txt

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

device_tree:
	cd device-tree && make all

install_module:
	sudo insmod /lib/modules/$(shell uname -r)/kernel/drivers/i2c/bmp280.ko

remove_module:
	sudo rmmod bmp280

read:
	sudo cat /dev/temp_sensor/bmp280

copy_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "rm -rf tp_td3"
	sshpass -p temppwd scp -r $(PWD) debian@192.168.7.2:/home/debian/tp_td3
	sshpass -p temppwd ssh debian@192.168.7.2

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
