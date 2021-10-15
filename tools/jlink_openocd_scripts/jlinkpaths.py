from pathlib import Path

address_bootloader             = hex(0x1000)
address_partition_table        = hex(0x8000)
address_firmware               = hex(0x10000)

_here = Path(__file__).resolve().parent
_path_project = _here.parent.parent

path_project                    = str(_path_project)
path_openocd_scripts_location   = str(_here.parent / "openocd" / "share" / "openocd" / "scripts")
path_openocd_executable         = str(_here.parent / "openocd" / "bin" / "openocd")
path_openocd_config             = str(_here / ".jlinkesp32.cfg")
path_gdbinit                    = str(_here / ".jlinkgdbinit")
path_build_dir                  = str(_path_project / "build")

_path_build_dir = Path(path_build_dir).resolve()

path_compiled_bootloader      = str(_path_build_dir / "bootloader" / "bootloader.bin")
path_compiled_partition_table = str(_path_build_dir / "partition_table" / "partition-table.bin")
path_compiled_firmware        = str(_path_build_dir / "esp32.bin")
path_compiled_firmware_elf    = str(_path_build_dir / "esp32.elf")
