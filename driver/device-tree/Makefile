#
# Makefile for the device tree file
# brief: This file is used to compile the device tree file and move it to the destination directory

# Specify the source file and output file names
SRC := am335x-boneblack.dts
OUT := am335x-boneblack.dtb

# Specify the destination directory
DEST := /boot/dtbs/$(shell uname -r)/

# Specify the backup directory
BACKUP := backup/

# Define the default target
all: $(OUT) install

# Rule to compile the device tree file
$(OUT): $(SRC)
	dtc -O dtb -o $(OUT) -b 0 -@ $(SRC)

# Rule to move the compiled device tree file to the destination directory
install: $(OUT)
	sudo cp $(OUT) $(DEST)

# Rule to save the original device tree file in the backup directory
backup:
	mkdir -p $(BACKUP)
	sudo cp $(DEST) $(BACKUP)

# Rule to clean the compiled device tree file
clean:
	rm -f $(OUT)