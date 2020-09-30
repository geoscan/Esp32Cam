#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import uavcan

args = argparse.ArgumentParser()
args.add_argument("--source-dir", dest="sourceDir", help="List of root namespace directories to parse", default="./uavcan/geoscan")
options = args.parse_args()

types = uavcan.dsdl.parse_namespaces([options.sourceDir])

for type in types:
	print(type)
	print("{0:#x}".format(type.get_data_type_signature()))