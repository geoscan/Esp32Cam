#!/bin/bash

# date=`date +%Y-%m-%d`
date=`date +D%Y%m%dT%H%M`
properties=par.properties
prefix=EspParFw
name=$prefix-`date +%Y-%m-%d-%H-%M`

rm -r -f *bin *properties *zip ./$prefix*

./extract_binaries.sh
# wget -O par.properties https://pioneer-doc.readthedocs.io/ru/master/_downloads/eced3253338b09e60e628c48571a617c/Pioneer-mini-release-1.0.0003.properties
wget -O $properties https://pioneer-doc.readthedocs.io/ru/master/_downloads/9f141dd22681fe536e57ce7bd4043e9d/Pioneer-mini-esp-test-1.0.0006.properties

sed -i 's/Copter_landedState=1.0/Copter_landedState=0.0/g' $properties

zip $name.zip *bin $properties

rm *bin *properties
