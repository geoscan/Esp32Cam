#!/bin/bash

source ./const.sh

# Build
cd $dirproj
source "./../$dirlib/export.sh"
idf.py clean
idf.py set-target esp32
idf.py build

source ./../acm.sh
# Flash
idf.py -p $devname -b 115200 flash

cd ..
