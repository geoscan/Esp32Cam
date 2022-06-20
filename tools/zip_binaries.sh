#!/bin/bash

tag=`git describe`
zip esp32-`date +%Y%m%d-%H%M`-$tag.zip *bin
