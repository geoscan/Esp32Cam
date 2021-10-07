#!/bin/bash

cd ..
./clear_binaries.sh
./extract_binaries.sh
cd -
mv ./../*.bin .

njet \
	--conn_type serial \
	--target pioneer \
	--esp_baudrate 57600 \
	--esp_firmware esp32.bin \
	--esp_bootloader bootloader.bin \
	--esp_parttable partition-table.bin \
	--esp_erase \
	--pl_baudrate 57600 \
	--pl_esp_reset_bootloader