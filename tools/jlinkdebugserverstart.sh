#!/bin/bash

HERE=$(dirname "${BASH_SOURCE[0]}")
HERE=$(realpath $HERE)
PYTHON=/usr/bin/python3

$PYTHON $HERE/jlink_openocd_scripts/jlinkgdbrun.py
