#!/bin/bash

echo Removing...
rm -f *.bin

echo Extracting...
mkdir firmware
dir_build="./../build"
cp $dir_build/esp32.bin ./firmware
cp $dir_build/partition_table/partition-table.bin ./firmware
cp $dir_build/bootloader/bootloader.bin ./firmware


