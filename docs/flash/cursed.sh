#!/bin/bash

source ./const.sh
source ./acm.sh

cd $dirtools/controller

./cursed_gui.py --serial $devname --baudrate 2000000
