PATH_IDF_PATH:=$(shell pwd)/espidf
PATH_IDF_TOOLS_PATH := $(shell pwd)/tools/idftools
PATH_XTS_COMPILER := $(shell pwd)/$(PATH_IDF_TOOLS_PATH)/xtensa-esp32-elf/esp-2021r1-8.4.0/xtensa-esp32-elf/bin

# The ESP-IDF framework relies on certain paths being 
# present in the environment. This snippet serves as
# a shortcut setting them up
SNIPPET_EXPORT_PATHS := \
	export PATH=$(PATH_XTS_COMPILER):$$PATH && \
	export IDF_PATH=$(PATH_IDF_PATH) && \
	export IDF_TOOLS_PATH=$(PATH_IDF_TOOLS_PATH)

# The chain evaluates as build -> build_preconfigured -> build_
# All the calls that get "proxied" through %_preconfigured must 
# abide the naming rule, i.e. "<TARGET_NAME>_" (note the underscore)

build: build_preconfigured
build_:
	idf.py build

clean: clean_preconfigured
clean_:
	idf.py clean

# Expands the stem $* automatic variable. Calls a new instance of make recursively
 %_preconfigured:
	$(SNIPPET_EXPORT_PATHS) && \
		. $(PATH_IDF_PATH)/export.sh && \
		$(MAKE) $*_

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
