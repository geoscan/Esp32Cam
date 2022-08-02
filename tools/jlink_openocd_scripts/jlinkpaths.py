from pathlib import Path
import copy

address_bootloader             = hex(0x1000)
address_partition_table        = hex(0x8000)
address_firmware               = hex(0x10000)

_here = Path(__file__).resolve().parent
_path_project = _here.parent.parent

openocd_use_from_toolchain = False  # There is a version of "Openocd" in the project, but it has some SEGFAULT bug which reveals itself from time to time

if openocd_use_from_toolchain:
	path_openocd_toolchain_base = _here.parent / "idftools" / "tools" / "openocd-esp32"  # Search in the toolchains directory
	path_openocd_toolchain_base = list(path_openocd_toolchain_base.glob("./*/openocd-esp32/"))
	assert len(path_openocd_toolchain_base) == 1  # It is expected to only have one openocd installed. Maybe, there is something wrong with the installed toolchain's directory structure
	path_openocd_toolchain_base = path_openocd_toolchain_base[0]
	path_openocd_executable = str(path_openocd_toolchain_base / "bin" / "openocd")
	path_openocd_scripts_location = str(path_openocd_toolchain_base / "share" / "openocd" / "scripts")
else:
	path_openocd_scripts_location   = str(_here.parent / "openocd" / "share" / "openocd" / "scripts")
	path_openocd_executable         = str(_here.parent / "openocd" / "bin" / "openocd")

path_project                    = str(_path_project)
path_openocd_config             = str(_here / ".jlinkesp32.cfg")

if openocd_use_from_toolchain:
	path_gdbinit                    = str(_here / ".jlinkgdbinit_toolchain")
else:
	path_gdbinit                    = str(_here / ".jlinkgdbinit")

path_build_dir                  = str(_path_project / "build")

_path_build_dir = Path(path_build_dir).resolve()

path_compiled_bootloader      = str(_path_build_dir / "bootloader" / "bootloader.bin")
path_compiled_partition_table = str(_path_build_dir / "partition_table" / "partition-table.bin")
path_compiled_firmware        = str(_path_build_dir / "esp32.bin")
path_compiled_firmware_elf    = str(_path_build_dir / "esp32.elf")

localvars = copy.copy(dict(locals()))

for k, v in localvars.items():
	if k.find("path_") == 0:
		print("{:>40} {}".format(k, str(v)))
