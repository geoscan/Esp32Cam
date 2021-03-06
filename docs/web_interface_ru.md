[TOC]

# Web-интерфейс для управления ESP32

## `192.168.4.1`

ESP32 создает подсеть, в которой ей (ESP32)  присваивается постоянный адрес `192.168.4.1`

## Информация о версии

Запрос: `http://192.168.4.1/info`.

Ответ: JSON вида.

```json
["0.2.8-59", "1.6.7481"]
```

### Структура JSON-массива

| Позиция | Описание                                                     |
| ------- | ------------------------------------------------------------ |
| `[1]`   | Версия прошивки ESP32                                        |
| `[2]`   | Версия прошивки платы, для которой ESP32 служит модулем связи (вероятнее всего - плата автопилота) <br> **Ограничения:** <br>- Может отсутствовать, если ESP32 не удалось получить информацию о версии |

## Информация о состоянии

Запрос.

```http
http://192.168.4.1/control
```

В ответ страница присылает JSON с описанием текущего состояния коптера вида:

```json
{
	"video_record":	false
}
```

### Поля JSON-ответа

| Поле                  | Значение                                                 |
| --------------------- | -------------------------------------------------------- |
| `video_record` : bool | `true` - запись ведется <br> `false` - запись не ведется |

## Управление состоянием - общие сведения

Управление производится отправкой *GET*-запросов вида:

```http
http://192.168.4.1/control?ARGUMENTS...
```

В ответ на команды управления коптер присылает JSON вида:

```json
{
	"video_record":	false,
	"success":	false,
	"message":	"Wrong input argument(s)"
}
```

Полученный JSON содержит инфромацию о результате выполнения команды.

Полученный JSON также содержит информацию об актуальном состоянии ESP32.

Структура полученного JSON'а справедлива для всех команд управления, если не указано обратное.

### Поля JSON-ответа

| Поле              | Значение                                                     |
| ----------------- | ------------------------------------------------------------ |
| `success`: bool   | `true` - команда выполнена успешно, <br/>`false` - команда не выполнена успешно |
| `message`: string | Появляется в случае `success: false`. <br>Содержит информацию о причине  провала. |

## Запись видео

### Запуск

Запрос:

```http
http://192.168.4.1/control
    ?function=video_record
    &command=start
    &name=<FILE_NAME>
```

Параметры:

| Параметр | Значение                                                     |
| -------- | ------------------------------------------------------------ |
| `name=`  | Имя сохраняемого файла<br/>**Ограничения:** <br/> - Максимальная длина - 8 символов |

### Остановка

Запрос:

```http
http://192.168.4.1/control
    ?function=video_record
    &command=stop
```

## Сохранение фото

Запрос:

```http
http://192.168.4.1/control
    ?function=photo
    &name=<FILE_NAME>
```

Параметры:

| Параметр                 | Значение                                                     |
| ------------------------ | ------------------------------------------------------------ |
| `name=FILE_NAME`: string | Имя сохраняемого файла <br>**Ограничения:** <br> - Максимальная длина - 8 символов |

