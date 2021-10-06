2021-03-05

# Работа с UDP/8001 (MAVLink Socket)

* ESP32 служит мостом между UART и WiFi.
* ESP32 открывает UDP порт 8001. 
* ESP32 поддерживает список клиентов. Сообщения с UART пересылаются каждому клиенту, и обратно.
* С каждым клиентом ассоцируется таймаут. По истечению таймаута клиент исключается из списка и больше не получает сообщений с UART

```sequence
UART->UART: <init>
ESP32->ESP32: <init UDP 8001>
UART->ESP32: MSG0
ESP32->ESP32: <no clients>
Client1->ESP32: MSG1
ESP32->UART: MSG1
UART->ESP32: MSG2
ESP32->Client1: MSG2
ESP32->ESP32: Client1 timeout
UART->ESP32: MSG3
ESP32->ESP32: <no clients>

```



# Работа с камерой


```sequence
Copter->Copter: Открывает TCP/8888
Copter->Copter: Открывает UDP/Y
Client->Client: Открывает TCP/X
Client->Client: Открывает UDP/X
Client->Copter: Коннект к Copter:TCP/8888 с Client:TCP/X
Copter->Copter: Accept
Copter->Client: JPEG с UDP/Y на UDP/X
Copter->Client: JPEG с UDP/Y на UDP/X
Copter->Client: JPEG с UDP/Y на UDP/X
Copter->Client: JPEG с UDP/Y на UDP/X
Copter->Client: ...
Client->Copter: Отключение Client:TCP/X от Copter:TCP/8888
Copter->Copter: Перестает отправлять JPEG
```

Обратите внимание, что клиент открывает 2 порта, `TCP/X`, `UDP/X` с одинаковым номером. По номеру TCP-порта клиента коптер определит номер UDP-порта клиента.

## Концепт

В данном случае `TCP/8888` служит флагом, обозначающим, что соединение поддерживается.
