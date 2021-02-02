#!/bin/bash

cmd_esptool="python3 ./etool/esptool.py"

./extract_binaries.sh
$cmd_esptool umux
$cmd_esptool reset
sleep 10
$cmd_esptool flash
