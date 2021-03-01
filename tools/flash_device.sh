#!/bin/bash

cmd_esptool="python3 ./etool/interface.py"

. ./extract_binaries.sh
$cmd_esptool pioneer_mini esp32boot
$cmd_esptool esp32 write -f esp32.bin