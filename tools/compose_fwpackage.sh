#!/bin/bash

# date=`date +%Y-%m-%d`
date=`date +D%Y%m%dT%H%M`
properties=par.properties
prefix=EspParFw
name=$prefix-`date +%Y-%m-%d-%H-%M`

rm -r -f *bin *properties *zip ./$prefix*

./extract_binaries.sh
wget -O par.properties https://pioneer-doc.readthedocs.io/ru/master/_downloads/eced3253338b09e60e628c48571a617c/Pioneer-mini-release-1.0.0003.properties
zip $name.zip *bin $properties

rm *bin *properties
