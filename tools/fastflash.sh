#!/bin/bash

_HERE=$(dirname $(realpath ${BASH_SOURCE[0]}))
_BAUDRATE=2000000

if [ "$#" -eq 1 ] ; then
	PORT=/dev/ttyUSB"$1"
else
	PORT=${PORT="/dev/ttyUSB0"}
fi

echo Port: $PORT

cd $_HERE/.. && \
	. environment.sh && \
	idf.py flash -b $_BAUDRATE -p $PORT
