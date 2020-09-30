#!/bin/bash

devid=`ls -l /dev | grep ACM | cut -d':' -f 2 | cut -d'M' -f 2`
devname=/dev/ttyACM$devid
sudo fuser -k $devname
sudo chmod a+rw $devname
