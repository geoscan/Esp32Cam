#!/usr/bin/python3

from jlinkpaths import *
import os

reset_after_every_flash = True
halt_on_main            = False
flash_bootloader        = True
flash_parttable         = True

mainfunc = "app_main"

command_reset            = "monitor reset halt"      if reset_after_every_flash else ""
command_halt_main        = f"""thb {mainfunc}\n c""" if halt_on_main            else ""
command_program          = "monitor program_esp32" 
command_flash_firmware   = f"{command_program} {path_compiled_firmware} {address_firmware} verify reset"
command_flash_bootloader = f"{command_program} {path_compiled_bootloader} {address_bootloader} verify reset"           if flash_bootloader else ""
command_flash_parttable  = f"{command_program} {path_compiled_partition_table} {address_partition_table} verify reset" if flash_parttable  else ""


gdbcommandscript = f"""
target remote :3333

set remote hardware-breakpoint-limit 2

file {path_compiled_firmware_elf}

{command_flash_bootloader}
{command_reset}

{command_flash_parttable}
{command_reset}

{command_flash_firmware}
{command_reset}

thb app_main
monitor reset halt
c
"""

def prepare_gdbinit():
    print(path_gdbinit)
    with open(path_gdbinit, 'w') as gdbinit:
        gdbinit.write(gdbcommandscript)

if __name__ == "__main__":
    prepare_gdbinit()
    os.system(f"xtensa-esp32-elf-gdb" + f" --command={path_gdbinit}")