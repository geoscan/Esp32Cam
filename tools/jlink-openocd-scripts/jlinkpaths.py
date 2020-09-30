from pathlib import Path

address_bootloader             = hex(0x1000)
address_partition_table        = hex(0x8000)
address_firmware               = hex(0x10000)

path_home                       = str(Path.home())
path_project                    = f"{path_home}/Documents/ESP32/esp32-firmware"
path_openocd_scripts_location   = f"{path_home}/.local/bin/openocd-esp32/share/openocd/scripts"
path_openocd_executable         = f"{path_home}/.local/bin/openocd-esp32/bin/openocd"
path_openocd_config             = f"{path_home}/.jlinkesp32.cfg"
path_gdbinit                    = f"{path_home}/.jlinkgdbinit"
path_build_dir                  = f"{path_project}/build"
path_xtensa_binaries            = f"{path_home}/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin"

path_compiled_bootloader        = f"{path_build_dir}/bootloader/bootloader.bin"
path_compiled_partition_table   = f"{path_build_dir}/partition_table/partition-table.bin"
path_compiled_firmware          = f"{path_build_dir}/esp32.bin"
path_compiled_firmware_elf      = f"{path_build_dir}/esp32.elf"
