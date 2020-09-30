# Инструментарий для прошивки ESP32
## Установка директории
Открыть консоль в любой директории. 
```bash
git clone --recursive https://github.com/damurashov/espcomplete.git tools
cd tools
```

Затем

```bash
./install_env.sh
```

## Подготовка к прошивке
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