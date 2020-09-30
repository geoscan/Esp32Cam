# Инструментарий для прошивки ESP32

## Требования
* Менеджер пакетов apt
* python
* python3
* python-pip
* python3-pip

### Pip[3]:
* pyserial
* numpy

## Начало

Выполнить

```bash
./install_env.sh
```

## Подготовка к прошивке
Включить коптер.

```bash
./cursed.sh
```
* Нажать 2
* Нажать `r`
* Нажать `/`
* Выполнить поиск по ключевому слову `mux`
* Выбрать параметр `BoardPioneerMini_modules_uMux`
* Выставить параметр `BoardPioneerMini_modules_uMux` = `2`


* Перезапустить коптер
* Подождать, пока LED на плате начнет быстро мигать

## Прошивка
```bash
./buildflash.sh
```

Перезапустить коптер