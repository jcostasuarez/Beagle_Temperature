# Nombre: Juan Costa Suárez
# Autor: Juan Costa Suárez
# Fecha: 10/11/2023
all: copy_to_board run

run:
	cd tp_td3
	cd driver
	make
connect_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2
copy_to_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "rm -rf tp_td3"
	sshpass -p temppwd scp -r $(PWD) debian@192.168.7.2:/home/debian/tp_td3
	sshpass -p temppwd ssh debian@192.168.7.2
open_dmesg_board:
	sshpass -p temppwd ssh debian@192.168.7.2 "dmesg -wH"