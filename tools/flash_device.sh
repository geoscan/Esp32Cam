#!/bin/bash

./extract_binaries.sh
mv *.bin ./esptool
cd esptool
python3 esptool.py
rm ./*.bin