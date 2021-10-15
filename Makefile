EXE_PYTHON := python3

PATH_IDF_PATH:=$(shell pwd)/espidf
PATH_IDF_TOOLS_PATH := $(shell pwd)/tools/idftools
PATH_XTS_COMPILER = $(wildcard $(PATH_IDF_TOOLS_PATH)/tools/xtensa-esp32-elf/*esp*)/bin
PATH_OPENOCD := $(wildcard $(PATH_IDF_TOOLS_PATH)/tools/openocd-esp32/*esp*)/openocd-esp32/bin
PATH_JLINK_SCRIPTS := $(shell pwd)/tools/jlink_openocd_scripts

# The ESP-IDF framework relies on certain paths being 
# present in the environment. This snippet serves as
# a shortcut setting them up
SNIPPET_EXPORT_PATHS := \
	export PATH=$(PATH_XTS_COMPILER):$(PATH_OPENOCD):$$PATH && \
	export IDF_PATH=$(PATH_IDF_PATH) && \
	export IDF_TOOLS_PATH=$(PATH_IDF_TOOLS_PATH)

# The chain evaluates as build -> build_preconfigured -> build_
# All the calls that get "proxied" through %_preconfigured must 
# abide the naming rule, i.e. "<TARGET_NAME>_" (note the underscore)

build: build_preconfigured
build_:
	idf.py build

rebuild: build_preconfigured

clean: clean_preconfigured
clean_:
	idf.py clean

jlinkespconnect: jlinkespconnect_preconfigured 
jlinkespconnect_:
	$(EXE_PYTHON) $(PATH_JLINK_SCRIPTS)/jlinkesp32.py

jlinkgdbconnect: build jlinkgdbconnect_preconfigured
jlinkgdbconnect_:
	$(EXE_PYTHON) $(PATH_JLINK_SCRIPTS)/jlinkgdbrun.py

# Expands the stem $* automatic variable. Calls a new instance of make recursively
%_preconfigured:
	@echo ---------------------------------
	@echo " $*"
	@echo ---------------------------------

	$(SNIPPET_EXPORT_PATHS) && \
		. $(PATH_IDF_PATH)/export.sh && \
		$(MAKE) $*_

	@echo ---------------------------------
	@echo " $* - SUCCESS"
	@echo ---------------------------------

# Clean / deconfigure the project completely, remove all installed dependencies and tools
distclean:
	@echo -- Cleaning everything
	rm -rf build
	rm -rf $(PATH_IDF_TOOLS_PATH)
	git submodule deinit --all --force

# Configure the project
configure:
	@echo -- Initializing submodules
	git submodule update --init --recursive

	@echo -- Installing ESP IDF
	$(SNIPPET_EXPORT_PATHS) && $(PATH_IDF_PATH)/install.sh

#.PHONY: build_preconfigured clean_preconfigured jlinkespconnect jlinkgdbconnect rebuild