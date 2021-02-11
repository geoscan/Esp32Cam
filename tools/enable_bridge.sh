#!/bin/bash

cmd_esptool="python3 ./etool/esptool.py"
$cmd_esptool umux
$cmd_esptool reset