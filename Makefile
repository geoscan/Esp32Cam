PATH_IDF_PATH:=$(shell pwd)/espidf
PATH_IDF_TOOLS_PATH := $(shell pwd)/tools/idftools
PATH_XTS_COMPILER = $(wildcard $(PATH_IDF_TOOLS_PATH)/tools/xtensa-esp32-elf/*esp*)/bin
PATH_OPENOCD := $(wildcard $(PATH_IDF_TOOLS_PATH)/tools/openocd-esp32/*esp*)/openocd-esp32/bin
PATH_JLINK_SCRIPTS := $(shell pwd)/tools/jlink_openocd_scripts
PATH_NJET := $(shell pwd)/tools/njet

PATH_COMPILED_BOOTLOADER := $(shell pwd)/build/bootloader/bootloader.bin
PATH_COMPILED_PARTTABLE := $(shell pwd)/build/partition_table/partition-table.bin
PATH_COMPILED_FIRMWARE := $(shell pwd)/build/esp32.bin

PATH_ENVIRONMENT := environment.sh

REMOVE_LIST := build $(PATH_IDF_TOOLS_PATH) $(PATH_ENVIRONMENT)
PRINT_PREFIX := "\\n\\n\\n------------------ "

EXE_PYTHON := python3
EXE_NJET := $(PATH_NJET)/njet

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

new_component:
	mkdir -p components/$(COMPONENT_FRAME)/$(COMPONENT_FRAME)
	cp tools/component_frame/CMakeLists.txt components/$(COMPONENT_FRAME)
	cp tools/component_frame/Kconfig.projbuild components/$(COMPONENT_FRAME)
	cp tools/component_frame/component_frame.cpp components/$(COMPONENT_FRAME)/$(COMPONENT_FRAME)/$(COMPONENT_FRAME).cpp
	cp tools/component_frame/component_frame.hpp components/$(COMPONENT_FRAME)/$(COMPONENT_FRAME)/$(COMPONENT_FRAME).hpp
	export COMPONENT_FRAME_DATE=`date +%Y-%m-%d` && find components/$(COMPONENT_FRAME) -type f | xargs -n 1 sed -i "s/COMPONENT_FRAME_DATE/$${COMPONENT_FRAME_DATE}/g"
	export COMPONENT_FRAME_UPPER=`echo $(COMPONENT_FRAME) | tr a-z A-Z` && find components/$(COMPONENT_FRAME) -type f | xargs -n 1 sed -i "s/COMPONENT_FRAME_UPPER/$${COMPONENT_FRAME_UPPER}/g"
	find components/$(COMPONENT_FRAME) -type f | xargs -n 1 sed -i 's/COMPONENT_FRAME_NAMESPACE/$(COMPONENT_FRAME_NAMESPACE)/g'
	find components/$(COMPONENT_FRAME) -type f | xargs -n 1 sed -i 's/COMPONENT_FRAME/$(COMPONENT_FRAME)/g'

clean: clean_preconfigured
clean_:
	idf.py clean

menuconfig: menuconfig_preconfigured
menuconfig_:
	idf.py menuconfig

pyclean: pyclean_preconfigured
pyclean_:
	idf.py python-clean

jlinkespconnect: jlinkespconnect_preconfigured
jlinkespconnect_:
	$(EXE_PYTHON) $(PATH_JLINK_SCRIPTS)/jlinkesp32.py

jlinkgdbconnect: jlinkgdbconnect_preconfigured
jlinkgdbconnect_:
	$(EXE_PYTHON) $(PATH_JLINK_SCRIPTS)/jlinkgdbrun.py

# Flash pioneer board
flashpioneer: build
	$(EXE_NJET) \
		--conn_type serial \
		--target pioneer \
		--esp_baudrate 57600 \
		--esp_firmware $(PATH_COMPILED_FIRMWARE) \
		--esp_bootloader $(PATH_COMPILED_BOOTLOADER) \
		--esp_parttable $(PATH_COMPILED_PARTTABLE) \
		--esp_erase \
		--pl_baudrate 57600 \
		--pl_esp_reset_bootloader

# Flash pioneer mini
flashmini: build
	$(EXE_NJET) \
		--conn_type serial \
		--target pioneer_mini \
		--esp_baudrate 115200 \
		--esp_firmware $(PATH_COMPILED_FIRMWARE) \
		--esp_bootloader $(PATH_COMPILED_BOOTLOADER) \
		--esp_parttable $(PATH_COMPILED_PARTTABLE) \
		--esp_erase \
		--pl_esp_reset_bootloader


# Expands the stem $* automatic variable. Calls a new instance of make recursively
%_preconfigured:
	@echo $(PRINT_PREFIX) " $*"

	$(SNIPPET_EXPORT_PATHS) && \
		. $(PATH_IDF_PATH)/export.sh && \
		$(MAKE) $*_

	@echo $(PRINT_PREFIX) " $* - SUCCESS"

# Clean / deconfigure the project completely, remove all installed dependencies and tools
distclean:
	@echo $(PRINT_PREFIX) Cleaning everything
	rm -rf $(REMOVE_LIST)
	git submodule deinit --all --force

# Configure the project
configure:
	@echo $(PRINT_PREFIX) Initializing submodules
	git submodule update --init --recursive

	@echo $(PRINT_PREFIX) Installing ESP IDF
	$(SNIPPET_EXPORT_PATHS) && $(PATH_IDF_PATH)/install.sh
	echo "$(SNIPPET_EXPORT_PATHS) && . $(PATH_IDF_PATH)/export.sh" > $(PATH_ENVIRONMENT)

	@echo $(PRINT_PREFIX) SUCCESS
	@echo Configuring done
	@echo Use \"make *\" or \". $(PATH_ENVIRONMENT)\" and further calls to manage your project
	@echo The latter one will give you access to the entire ESP-IDF toolset, you are encouraged
	@echo to use it

#.PHONY: build_preconfigured clean_preconfigured jlinkespconnect jlinkgdbconnect rebuild
