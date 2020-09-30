#!/bin/bash

source ./const.sh

# Clone and install esp framework

git clone --recursive https://github.com/espressif/esp-idf.git $dirlib
cd $dirlib
git checkout v4.3-dev
chmod +x ./install.sh
./install.sh
git submodule update --init --recursive
cd ..

# Clone the project itself

#git clone https://github.com/damurashov/tempesp.git $dirproj
git clone --recursive git@gitlab.corp.geoscan.aero:pioneermini/esp32cam.git $dirproj
cd $dirproj
git checkout esp32firm
git submodule update --init --recursive
cd ..

# Clone cursed gui tool

#git clone https://github.com/damurashov/minitools.git $dirtools
