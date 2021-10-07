# Прошивка для ESP32
[TOC]

# Зависимости

## esp-idf framework

* [Windows](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html)
* [Хорошая официальная инструкция](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

## TL; DR linux

Воспринимайте этот мануал как *вспомогательный*. Официальная инструкция точнее и полнее.

1. Инициализировать подмодули

```bash
git submodule update --init --recursive
```

2. Вместе с подмодулями в директорию `espidf/` скопируется фреймворк `esp-idf`, с использованием которого написана прошивка. Запустите установку необходимых инструментов.

```bash
cd espidf
install.sh
```

3. Работа с `esp-idf` во многом автоматизируется python-скриптами, поставляемыми вместе с самим фреймворком. Фреймворки полагаются на системную переменную `IDF_PATH`, указывающую на папку `esp-idf`, склонированную из `github.com/espressif/esp-idf`.

   Добавление переменной `$IDF_PATH`

```bash
export IDF_PATH=`pwd`
```

4. Перед каждой сборкой надо настраивать окружение. `esp-idf` это автоматизирует.

```bash
source $IDF_PATH/export.sh
```

5. Запуск сборки

```bash
idf.py build
```

```
idf.py build
Executing action: all (aliases: build)
Running cmake in directory /home/dmurashov/Documents/ESP32/esp32-firmware/build
Executing "cmake -G Ninja -DPYTHON_DEPS_CHECKED=1 -DESP_PLATFORM=1 --warn-uninitialized -DIDF_TARGET=esp32 -DCCACHE_ENABLE=0 /home/dmurashov/Documents/ESP32/esp32-firmware"...
Warn about uninitialized values.
-- Found Git: /usr/bin/git (found version "2.17.1")
-- Component directory /home/dmurashov/Documents/ESP32/esp-idf/components/tinyusb does not contain a CMakeLists.txt file. No component will be added
-- The C compiler identification is GNU 8.2.0
-- The CXX compiler identification is GNU 8.2.0
-- The ASM compiler identification is GNU
-- Found assembler: /home/dmurashov/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /home/dmurashov/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /home/dmurashov/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-g++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
```

# Проект
## Сборка проекта

Из директории проекта, предварительно выполнив `source $IDF_PATH/export.sh`, запустить

```bash
idf.py build
```

Будут собраны `.bin`-файлы:

File | Descr. | Flash offset
---|---|---
`esp32.bin` | f/w | 0x10000
`partition_table.bin` | Partition Table | 0x8000
`bootloader.bin` | *2nd stage* bootloader ([подробнее про процесс стандартного bootloading'а ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/general-notes.html)) | 0x1000

## Конфигурирование проекта

Из директории проекта, предварительно выполнив `source $IDF_PATH/export.sh`, запустить
```bash
idf.py menuconfig
```

# Прошивка
В ROM ESP32 зашит *1st stage* bootloader, который переводит устройство в режим загрузки. Выбор режима осуществляется пином `IO0`. Пин `IO0` должен быть выставлен в нужное значение перед подачей питания.

GPIO0 Input     | Mode
--------------- | ----------------------------------------------
Low/GND         | ROM serial bootloader for esptool.py
High/VCC        | Normal execution mode

*Как вариант, для перевода в режим bootloading'а можно подсоединить `IO0` к `GND`.*

В режиме bootloading'а `ESP32` можно управлять через пины `RXD0` и `TXD0`. 

Для прошивки следует подключить ESP32 через последовательный порт. Можно использовать *USB-UART* переходник.

Прошивка осуществляется через `idf.py`

```bash
idf.py flash -b 115200 -p /dev/ttyACM0
```

Устройство может распознаваться *иначе* чем `/dev/ttyACM0`. Подставьте нужное значение.

В консоли выведется информация о процессе прошивки.


# JTAG: прошивка, отладка
## HW: подключение JTAG
Пины отсчитываются против часовой стрелки начиная с 1

&nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp;
--- | --- | --- | --- | ---
1 | &nbsp; | &nbsp; | &nbsp; | 38
... | &nbsp; | &nbsp; | &nbsp; | ...
14 | &nbsp; | &nbsp; | &nbsp; | 25
. | 15 | ... | 24 | .


Подключение JTAG к пинам:

ESP order | ESP name | JTAG order | JTAG name
---|---|---|---
1  | GND  | 4  | GND
2  | 3v3  | 1  | VTREF
3  | EN   | 3  | TRST
13 | IO14 | 7  | TMS
14 | IO12 | 5  | TDI
16 | IO13 | 9  | TCK
23 | IO15 | 13 | TDO

### Известные проблемы
#### `esp32.cpu0: IR capture error; saw 0x1f not 0x01`
Возможно, пины `12`-`15`, используемые для JTAG-подключения, сконфигурированы на момент попытки подключения. Попробуйте стереть память и попробовать заново.

## HW/SW: Segger JLink + OpenOCD + ESP32

Отладка/прошивка `ESP32` производится в связке `Segger JLink` + `OpenOCD`.

Для гарантии совместимости проверенная версия `OpenOCD` была добавлена к проекту, директория `tools/openocd`.

Для автоматизации работы с `OpenOCD` были написаны скрипты, папка `tools/jlink-openocd-scripts`. Предполагается, что на момент запуска скриптов **переменная `IDF_PATH` добавлена в окружение**.

* `jlinkesp32.py` - запуск GDB-сервера.
* `jlinkgdbrun.py` - запуск GDB-клиента, загрузка прошивки. Также полезен как reference при настройке процесса прошивки в IDE и как тест что все работает.
* `jlinkpaths.py` - константы, пути в файловой системе. Указывают на расположение проекта, `OpenOCD`, компилятора `XTensa`. Вам понадобится **изменить их под целевое окружение**.

## Работа с ESP32 в QtCreator
Стандартные средства интеграции средств отладки в QtCreator не работают. Используется схема подключения к удаленному отладчику.

**Важно**: не забудьте настроить Qt Kits и указать `xtensa-esp32-elf-gdb` в качестве отладчика.

1. В одном процессе запустите GDB-сервер `jlinkesp32.py`.
2. В *QtCreator* подключитесь к GDB-серверу
2.1 `Debug / Start Debugging / Attach to Running Debug Server...`
2.2 Добавьте настройки GDB-клиента

Field | Value
--- | ---
Server Port | 3333
Local Executable | `<project path>/build/esp32.elf`
Init commands | *см. ниже*
Reset commands | *см. ниже*

### Init commands
```gdb

set remote hardware-breakpoint-limit 2

file /home/dmurashov/Documents/ESP32/esp32-firmware/build/esp32.elf

monitor program_esp32 /home/dmurashov/Documents/ESP32/esp32-firmware/build/bootloader/bootloader.bin 0x1000 verify reset
monitor reset halt

monitor program_esp32 /home/dmurashov/Documents/ESP32/esp32-firmware/build/partition_table/partition-table.bin 0x8000 verify reset
monitor reset halt

monitor program_esp32 /home/dmurashov/Documents/ESP32/esp32-firmware/build/esp32.bin 0x10000 verify reset
monitor reset halt

```

Команды выполняют загрузку bootloader'а, прошивки и Partition Table. Если нет необходимости, Вы можете выполнять только загрузку `esp32.bin`.

### Reset commands
```gdb
thb app_main
monitor reset halt
c
```

*С первого раза может не подключиться. Повторите шаги 2.x.*
