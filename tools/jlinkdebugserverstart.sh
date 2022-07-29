#!/bin/bash

HERE=$(dirname "${BASH_SOURCE[0]}")
HERE=$(realpath $HERE)
cd $HERE/.. && make jlinkespconnect
PYTHON=/usr/bin/python3

$PYTHON $here/jlink_openocd_scripts/jlinkgdbrun.py &
