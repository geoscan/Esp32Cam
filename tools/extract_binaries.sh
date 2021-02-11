#!/bin/bash

echo Removing...
rm -f *.bin

echo Extracting...
dir_build="./../build"
cp $dir_build/esp32.bin .
cp $dir_build/partition_table/partition-table.bin .
cp $dir_build/bootloader/bootloader.bin .


