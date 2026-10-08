# AT-команды прошивки ESP-AT SPI

Справочник команд для модуля **ESP8684-MINI-1-H2X** с прошивкой
`ESP32C2-MINI-1-H2X-SPI`. Включены только команды, доступные в этой сборке.

Полный официальный синтаксис Espressif:
https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/index.html

Обзор возможностей модуля: `DOC/FIRMWARE_CAPABILITIES.md`.

---

## 0. Правила обмена

### Транспорт

- AT идёт **только через SPI**, не через UART.
- UART0 GPIO8/GPIO9 — только диагностический лог.
- Каждая AT-команда завершается `\r\n` (CR-LF).
- Длина одной AT-команды: не более **256 байт**.
- Строки заключаются в кавычки: `"ssid"`.
- Спецсимволы в параметрах экранируются: `\,`, `\"`, `\\`.
- Если пропускается необязательный параметр, запятая остаётся: `AT+CWJAP="a","b",,1`.

### Типы команд

| Тип | Формат | Смысл |
|---|---|---|
| Test | `AT+CMD=?` | Диапазоны параметров |
| Query | `AT+CMD?` | Текущее значение |
| Set | `AT+CMD=<...>` | Установка и выполнение |
| Execute | `AT+CMD` | Выполнение без параметров |

### Пассивные ответы

| Ответ | Значение |
|---|---|
| `OK` | Команда выполнена |
| `ERROR` | Ошибка |
| `SEND OK` | Данные приняты стеком (`CIPSEND` / HTTP POST) |
| `SEND FAIL` | Ошибка отправки данных |
| `SET OK` | URL задан (`HTTPURLCFG`) |
| `>` | ESP ждёт бинарные/сырые данные |

### Активные сообщения (важные для хоста)

| Сообщение | Значение |
|---|---|
| `ready` | Модуль готов |
| `busy p...` | Занят предыдущей командой |
| `WIFI CONNECTED` | STA подключена к AP |
| `WIFI GOT IP` | Получен IPv4 |
| `WIFI GOT IPv6 LL` | Получен IPv6 link-local |
| `WIFI GOT IPv6 GL` | Получен IPv6 global |
| `WIFI DISCONNECT` | STA отключена |
| `+STA_CONNECTED:<mac>` | Клиент подключился к SoftAP |
| `+DIST_STA_IP:<mac>,<ip>` | SoftAP выдал IP клиенту |
| `+STA_DISCONNECTED:<mac>` | Клиент ушёл с SoftAP |
| `[<id>,]CONNECT` | Соединение установлено |
| `[<id>,]CLOSED` | Соединение закрыто |
| `+IPD...` | Пришли сетевые данные |
| `+LINK_CONN...` | Подробности соединения |
| `+TIME_UPDATED` | Время обновлено по SNTP |
| `+QUITT` | Выход из passthrough |
| `ERR CODE:0x........` | Код ошибки (если включён `AT+SYSLOG`) |

Драйвер хоста должен обрабатывать асинхронные сообщения параллельно с ответами на команды.

### Ограничения этой прошивки

- Максимум **5** сетевых AT-соединений.
- HTTP TX/RX буфер по умолчанию: **2048** байт.
- SPI stream buffer: **2048** байт.
- **Нет**: MQTT, WebSocket, BLE, BluFi, EAP, FS, WebServer, Wi-Fi OTA (`AT+CIUPDATE`).
- `AT+CWJEAP` недоступна (Enterprise Wi-Fi выключен).
- `AT+CIUPDATE` / сетевое обновление прошивки **не использовать** в изделии: обновление только с хоста.

---

## 1. Базовые команды

### `AT`

Проверка связи.

```
AT
OK
```

### `AT+RST`

Перезапуск модуля. После рестарта снова придёт `ready`.

```
AT+RST
OK
```

### `AT+GMR`

Версия AT / SDK / compile time.

```
AT+GMR
...
OK
```

### `AT+CMD`

Список всех команд текущей прошивки и поддерживаемых типов (`?`, `=?`, `=`).

```
AT+CMD?
```

### `AT+RESTORE`

Сброс настроек к заводским и перезапуск.

```
AT+RESTORE
```

### `AT+GSLP=<time_ms>`

Deep sleep на заданное время (мс). Для типового прибора обычно не нужен.

### `AT+SLEEP`

Query/Set режима сна.

```
AT+SLEEP?
AT+SLEEP=<mode>
```

Типичные значения mode: `0` — без сна, `1`/`2`/`3` — режимы энергосбережения ESP-AT.

### `AT+SLEEPWKCFG`

Источник пробуждения из light sleep (GPIO и т.п.).

```
AT+SLEEPWKCFG=<wakeup_source>,<param1>[,<param2>]
```

### `AT+SYSSTORE=<mode>`

Сохранять ли параметры команд в NVS.

- `0` — не сохранять
- `1` — сохранять (по умолчанию для многих Wi-Fi/IP-команд)

```
AT+SYSSTORE?
AT+SYSSTORE=1
```

### `AT+SYSRAM?`

Свободная/минимальная heap-память. Полезно для диагностики.

```
AT+SYSRAM?
+SYSRAM:<free>,<min_free>
OK
```

### `AT+SYSMSG=<mask>`

Какие системные сообщения выводить (Wi-Fi / соединение и т.д.).

```
AT+SYSMSG?
AT+SYSMSG=<mask>
```

### `AT+SYSMSGFILTER` / `AT+SYSMSGFILTERCFG`

Включение фильтра сообщений и задание шаблонов фильтрации.

```
AT+SYSMSGFILTER=<enable>
AT+SYSMSGFILTERCFG=<...>
```

### `AT+SYSFLASH`

Чтение/запись пользовательских разделов Flash. Для обычной работы прибора не требуется.

### `AT+SYSMFG`

Чтение/запись manufacturing NVS (сертификаты, factory params и т.п.).

```
AT+SYSMFG?
AT+SYSMFG=<operate>,<"namespace">,<"key">[,...]
```

### `AT+RFPOWER`

Query/Set мощности RF TX.

```
AT+RFPOWER?
AT+RFPOWER=<wifi_power>
```

### `AT+RFCAL`

Полная RF-калибровка.

```
AT+RFCAL
```

### `AT+SYSTIMESTAMP`

Локальная метка времени (секунды).

```
AT+SYSTIMESTAMP?
AT+SYSTIMESTAMP=<unix_time>
```

### `AT+SYSLOG=<enable>`

Включить/выключить вывод `ERR CODE`.

```
AT+SYSLOG=1
```

### `AT+SYSREG`

Чтение/запись регистра SoC. Только для отладки.

### `AT+UART_CUR` / `AT+UART_DEF`

Конфигурация UART. В этой прошивке AT работает по SPI; команды могут существовать
для совместимости, но **не являются основным каналом управления**.

### `AT+SAVETRANSLINK`

Автозапуск passthrough при питании.

```
AT+SAVETRANSLINK=<mode>,<"remote host">,<remote port>[,<"type">][,<keepalive>]
```

### `AT+TRANSINTVL=<interval>`

Интервал передачи в passthrough.

### `AT+SYSROLLBACK`

Откат прошивки. В данной H2X-сборке один app-раздел `ota_0`, dual-OTA не используется;
для продукта полагаться на эту команду не следует.

---

## 2. Пользовательские команды

### `AT+USERRAM`

Работа со служебным RAM-буфером AT.

```
AT+USERRAM?
AT+USERRAM=<op>,<size>[,<offset>]
```

`op`: `0` free, `1` malloc, `2` write, `3` read, `4` clear.

### `AT+USERWKMCUCFG`

Как ESP будит хост-контроллер перед исходящими данными.

```
AT+USERWKMCUCFG=<enable>,<wake_mode>,<wake_number>,<wake_signal>,<delay_ms>[,<check_method>]
```

- `wake_mode=1` — GPIO
- `wake_mode=2` — UART byte

### `AT+USERMCUSLEEP=<state>`

Хост сообщает ESP своё состояние сна: `0` awake, `1` sleep.

### `AT+USERDOCS?`

URL документации текущей прошивки.

### `AT+USEROTA`

Скачивание прошивки по URL (HTTP/HTTPS). **В изделии не использовать**:
политика обновления — только с хоста по SPI/esptool.

---

## 3. Wi-Fi команды

### `AT+CWINIT=<init>`

Инициализация / деинициализация Wi-Fi драйвера. Обычно `1`.

### `AT+CWMODE`

Режим Wi-Fi.

```
AT+CWMODE?
AT+CWMODE=<mode>[,<auto_connect>]
```

| mode | Значение |
|---:|---|
| 0 | Wi-Fi off / NULL |
| 1 | Station |
| 2 | SoftAP |
| 3 | Station + SoftAP |

Пример:

```
AT+CWMODE=1
```

### `AT+CWSTATE?`

Текущее состояние Wi-Fi и информация о подключении.

### `AT+CWBANDWIDTH`

Query/Set полосы Wi-Fi.

### `AT+CWCONFIG`

Inactive time / listen interval.

```
AT+CWCONFIG=<inactive_time>[,<listen_interval>]
```

### `AT+CWLAPOPT`

Формат вывода `AT+CWLAP`.

```
AT+CWLAPOPT=<sort_enable>,<mask>[,<rssi_filter>][,<authmode_mask>]
```

### `AT+CWLAP`

Сканирование сетей.

```
AT+CWLAP
+CWLAP:(<ecn>,<"ssid">,<rssi>,<"mac">,<channel>,...)
OK
```

Можно фильтровать:

```
AT+CWLAP=[<ssid_len>][,<"ssid">][,<mac>][,<channel>][,...]
```

### `AT+CWJAP`

Подключение к AP.

```
AT+CWJAP=<"ssid">,<"pwd">[,<"bssid">][,<pci_en>][,<recon_int>][,<listen_int>][,<scan_mode>][,<jap_timeout>][,<pmf>]
```

Минимум:

```
AT+CWJAP="MyAP","password"
WIFI CONNECTED
WIFI GOT IP
OK
```

Query:

```
AT+CWJAP?
+CWJAP:<"ssid">,<"bssid">,<channel>,<rssi>,...
OK
```

### `AT+CWQAP`

Отключение от AP.

```
AT+CWQAP
OK
```

### `AT+CWRECONNCFG`

Параметры автопереподключения STA.

```
AT+CWRECONNCFG=<interval_s>,<repeat_count>
```

### `AT+CWAUTOCONN=<enable>`

Автоподключение к последней AP после питания (`0/1`). Сохраняется в flash.

### `AT+CWSAP`

Конфигурация SoftAP.

```
AT+CWSAP=<"ssid">,<"pwd">,<channel>,<ecn>[,<max_conn>][,<ssid_hidden>]
AT+CWSAP?
```

`ecn`: 0 OPEN, 2 WPA_PSK, 3 WPA2_PSK, 4 WPA_WPA2_PSK, …

### `AT+CWLIF`

Список клиентов SoftAP и их IP.

```
AT+CWLIF
+CWLIF:<ip>,<mac>
OK
```

### `AT+CWQIF`

Отключить клиентов SoftAP.

```
AT+CWQIF
AT+CWQIF=<"mac">
```

### `AT+CWDHCP`

DHCP STA/SoftAP.

```
AT+CWDHCP?
AT+CWDHCP=<operate>,<mode>
```

`mode` — битовая маска интерфейсов (STA/SoftAP).

### `AT+CWDHCPS`

Диапазон DHCP SoftAP.

```
AT+CWDHCPS=<enable>,<lease>,<"start_ip">,<"end_ip">
```

### `AT+CWAPPROTO` / `AT+CWSTAPROTO`

Wi-Fi protocol bitmap SoftAP / Station (b/g/n …).

### `AT+CIPSTAMAC` / `AT+CIPAPMAC`

MAC станции / SoftAP.

```
AT+CIPSTAMAC?
AT+CIPSTAMAC=<"mac">
AT+CIPAPMAC?
AT+CIPAPMAC=<"mac">
```

### `AT+CIPSTA` / `AT+CIPAP`

IP/маска/шлюз STA / SoftAP.

```
AT+CIPSTA?
+CIPSTA:ip:"x.x.x.x"
+CIPSTA:gateway:"x.x.x.x"
+CIPSTA:netmask:"x.x.x.x"
OK

AT+CIPSTA=<"ip">[,<"gateway">][,<"netmask">]
AT+CIPAP=<"ip">[,<"gateway">][,<"netmask">]
```

### `AT+CWHOSTNAME`

Hostname станции.

```
AT+CWHOSTNAME=<"name">
AT+CWHOSTNAME?
```

### `AT+CWCOUNTRY`

Код страны Wi-Fi.

```
AT+CWCOUNTRY=<country_code>,<start_channel>,<total_channel_count>
AT+CWCOUNTRY?
```

### `AT+CWSTARTSMART` / `AT+CWSTOPSMART`

SmartConfig (AirKiss / ESP-Touch).

```
AT+CWSTARTSMART[=<type>][,<"crypt_key">]
AT+CWSTOPSMART
```

### `AT+WPS`

WPS.

```
AT+WPS=<enable>[,<auth_mode_threshold>]
```

`enable=1` старт, `0` стоп.

### Недоступно в этой сборке

- `AT+CWJEAP` — Wi-Fi Enterprise / EAP выключен.

---

## 4. TCP / UDP / SSL команды

### `AT+CIPV6=<enable>`

Включить/выключить IPv6 (`0/1`). В сборке IPv6 поддерживается.

### `AT+CIPMUX=<mode>`

- `0` — одно соединение
- `1` — множественные (до 5)

```
AT+CIPMUX=1
```

### `AT+CIPSTART`

Установить соединение.

**Одиночный режим (`CIPMUX=0`):**

```
AT+CIPSTART=<"type">,<"remote host">,<remote port>[,<keep_alive>][,<"local IP">][,<timeout>]
```

**Множественный (`CIPMUX=1`):**

```
AT+CIPSTART=<link_id>,<"type">,<"remote host">,<remote port>[,<keep_alive>][,<"local IP">][,<timeout>]
```

`type`:

| type | Назначение |
|---|---|
| `TCP` | TCP-клиент |
| `TCPv6` | TCP IPv6 |
| `UDP` | UDP |
| `UDPv6` | UDP IPv6 |
| `SSL` | TLS-клиент |
| `SSLv6` | TLS IPv6 |

Пример TCP:

```
AT+CIPSTART="TCP","192.168.1.10",5000
CONNECT
OK
```

Пример TLS:

```
AT+CIPSTART="SSL","example.com",443
```

Для UDP часто указывают локальный порт:

```
AT+CIPSTART="UDP","192.168.1.10",5000,5001,0
```

### `AT+CIPSTARTEX`

Как `CIPSTART`, но `link_id` назначается автоматически.

### `AT+CIPSTATE?`

Список активных соединений.

```
AT+CIPSTATE?
+CIPSTATE:<link_id>,<"type">,<"remote_ip">,<remote_port>,<local_port>,<tetype>
OK
```

### `AT+CIPDOMAIN`

DNS lookup.

```
AT+CIPDOMAIN=<"domain">[,<ip_network>][,<timeout>]
+CIPDOMAIN:<"IP">
OK
```

### `AT+CIPSEND`

Отправка данных.

```
AT+CIPSEND[=<link_id>],<length>
>
...data...
SEND OK
```

В passthrough (`CIPMODE=1`) после входа данные идут прозрачно.

### `AT+CIPSENDEX`

Отправка с расширенным синтаксисом (в т.ч. `\0` как конец).

### `AT+CIPSENDL` / `AT+CIPSENDLCFG`

Отправка длинных данных параллельно / настройка.

### `AT+CIPCLOSE`

```
AT+CIPCLOSE
AT+CIPCLOSE=<link_id>
```

### `AT+CIFSR`

Локальные IP/MAC.

```
AT+CIFSR
+CIFSR:STAIP,"x.x.x.x"
+CIFSR:STAMAC,".."
OK
```

### `AT+CIPSERVER`

TCP/SSL сервер.

```
AT+CIPSERVER=<mode>[,<port>][,<"type">][,<ca_enable>][,<"netif">]
```

- `mode=1` создать, `0` удалить
- `type`: `TCP` или `SSL`

Сначала обычно:

```
AT+CIPMUX=1
AT+CIPSERVER=1,8080
```

### `AT+CIPSERVERMAXCONN`

Максимум клиентов сервера.

```
AT+CIPSERVERMAXCONN=<num>
```

Не больше `AT_SOCKET_MAX_CONN_NUM` (=5 в этой сборке).

### `AT+CIPMODE=<mode>`

- `0` — нормальный AT-режим передачи
- `1` — passthrough (прозрачный)

Выход из passthrough: последовательность `+++` (без `\r\n`, с паузами) либо механизм Espressif для текущего интерфейса.

### `AT+CIPSTO=<timeout_s>`

Таймаут TCP-сервера.

### `AT+CIPDINFO=<mode>`

Добавлять ли remote IP/port в `+IPD` (`0/1`).

### `AT+CIPRECVTYPE`

Режим приёма сокета:

- `0` — активный (`+IPD` сразу с данными)
- `1` — пассивный (сначала длина, данные читать `CIPRECVDATA`)

```
AT+CIPRECVTYPE=<link_id>,<mode>
```

### `AT+CIPRECVLEN?`

Длина доступных данных в пассивном режиме.

### `AT+CIPRECVDATA`

Чтение данных в пассивном режиме.

```
AT+CIPRECVDATA=<link_id>,<len>
+CIPRECVDATA:<actual_len>,<data>
OK
```

### `AT+CIPCONNPERSIST`

Сохранять ли TCP/SSL соединение при реконфигурации сети.

### `AT+CIPRECONNINTV`

Интервал реконнекта в network transmission mode.

### `AT+CIPTCPOPT`

Опции сокета (So_Linger, TCP_Nodelay, So_Sndtimeo, Keepalive…).

```
AT+CIPTCPOPT=<link_id>,<so_linger>,<tcp_nodelay>,<so_sndtimeo>[,<keepalive>]
```

### `AT+CIPDNS`

DNS-серверы.

```
AT+CIPDNS?
AT+CIPDNS=<enable>[,<"DNS IP1">][,<"DNS IP2">][,<"DNS IP3">]
```

### `AT+PING=<"host">`

Ping.

```
AT+PING="8.8.8.8"
+PING:<time_ms>
OK
```

### `AT+MDNS`

Публикация mDNS / Bonjour.

```
AT+MDNS=<enable>[,<"hostname">,<"service_name">,<port>][,<"instance">][,<"protocol">][,<"txt_number">][,<"key">,<"value">]...
```

Пример:

```
AT+MDNS=1,"device","_http",8080
```

После этого устройство может находиться как `device.local` (в зависимости от hostname/сервиса и ОС).

### SNTP

```
AT+CIPSNTPCFG=<enable>[,<timezone>][,<"SNTP server1">][,<"SNTP server2">][,<"SNTP server3">]
AT+CIPSNTPTIME?
AT+CIPSNTPINTV=<seconds>
```

После синхронизации возможно сообщение `+TIME_UPDATED`.

### SSL/TLS клиент

```
AT+CIPSSLCCONF=<link_id>,<auth_mode>[,<pki_number>][,<ca_number>]
AT+CIPSSLCCIPHER=<link_id>,<"cipher suite">
AT+CIPSSLCCN=<link_id>,<"common name">
AT+CIPSSLCSNI=<link_id>,<"sni">
AT+CIPSSLCALPN=<link_id>,<alpn_counts>,<"alpn">...
AT+CIPSSLCPSK=<link_id>,<"psk">,<"hint">
AT+CIPSSLCPSKHEX=<link_id>,<"psk_hex">,<"hint">
```

`auth_mode`:

| Значение | Смысл |
|---:|---|
| 0 | без проверки |
| 1 | клиентский сертификат для сервера |
| 2 | проверка сервера по CA |
| 3 | взаимная проверка |

Для продакшена через Интернет рекомендуется `2` или `3`, а не `0`.

### Не использовать в изделии

- `AT+CIUPDATE` — Wi-Fi OTA выключен.
- `AT+CIPFWVER` — проверка версии на сервере OTA, для этой политики не нужна.

---

## 5. HTTP / HTTPS команды

Буферы по умолчанию в этой сборке: TX=2048, RX=2048.

### `AT+HTTPCLIENT`

Универсальный HTTP-запрос.

```
AT+HTTPCLIENT=<opt>,<content-type>,<"url">,[<"host">],[<"path">],<transport_type>[,<"data">][,<"header">]...
```

| Параметр | Значения |
|---|---|
| `opt` | 1 HEAD, 2 GET, 3 POST, 4 PUT, 5 DELETE |
| `content-type` | 0 form, 1 json, 2 multipart, 3 xml |
| `transport_type` | 1 HTTP, 2 HTTPS |

Ответ:

```
+HTTPCLIENT:<size>,<data>
OK
```

Примеры:

```
AT+HTTPCLIENT=2,0,"http://httpbin.org/get","httpbin.org","/get",1
AT+HTTPCLIENT=3,0,"http://httpbin.org/post","httpbin.org","/post",1,"a=1&b=2"
```

Если вся команда длиннее 256 байт — сначала `AT+HTTPURLCFG`.

### `AT+HTTPGETSIZE`

Размер ресурса.

```
AT+HTTPGETSIZE=<"url">[,<tx>][,<rx>][,<timeout_ms>]
+HTTPGETSIZE:<size>
OK
```

### `AT+HTTPCGET`

GET с телом ответа.

```
AT+HTTPCGET=<"url">[,<tx>][,<rx>][,<timeout_ms>]
+HTTPCGET:<size>,<data>
OK
```

### `AT+HTTPCPOST`

POST произвольной длины: после `>` отправляются `<length>` байт тела.

```
AT+HTTPCPOST=<"url">,<length>[,<header_cnt>][,<"header">...]
OK
>
...body...
SEND OK
```

### `AT+HTTPCPUT`

Аналогично POST, метод PUT.

```
AT+HTTPCPUT=<"url">,<length>[,<header_cnt>][,<"header">...]
```

### `AT+HTTPURLCFG`

Длинный URL (>256 в командной строке).

```
AT+HTTPURLCFG=<url_len>
>
...url...
SET OK
```

Далее в HTTP-командах указывать `""` как URL.

`url_len=0` очищает конфигурацию.

### `AT+HTTPCHEAD`

Глобальные HTTP-заголовки.

```
AT+HTTPCHEAD=<len>
>
Key: Value
```

`len=0` очищает все заголовки.

Query:

```
AT+HTTPCHEAD?
+HTTPCHEAD:<index>,<"header">
OK
```

### `AT+HTTPCFG`

Аутентификация HTTP-клиента сертификатами.

```
AT+HTTPCFG=<auth_mode>[,<pki_number>][,<ca_number>]
```

В этой сборке опции `AT_HTTPS_*_AUTH_*` по умолчанию выключены в menuconfig;
для строгой проверки сертификатов может потребоваться донастройка PKI и пересборка.

### `AT+HTTPCSNI`

SNI для HTTPS.

```
AT+HTTPCSNI=<"hostname">
AT+HTTPCSNI?
```

Пример:

```
AT+HTTPCSNI="api.example.com"
AT+HTTPCGET="https://api.example.com/v1/status"
```

### Ошибки HTTP

При `AT+SYSLOG=1` возможны:

```
+HTTPERR:<http_err>,<tls_err>,<cert_flags>,<sock_errno>
ERR CODE:0x010a7xxx
```

где `7xxx` часто соответствует HTTP status (например `0x7194` = 404).

---

## 6. Signaling (заводские/тестовые)

### `AT+FACTPLCP`

Выбор long/short PLCP для тестовых передач.

```
AT+FACTPLCP=<enable>,<tx_with_long>
```

Для прикладного ПО прибора обычно не нужна.

---

## 7. Рекомендуемые сценарии

### A. Подключение STA и TCP-клиент

```
AT
AT+GMR
AT+CWMODE=1
AT+CWJAP="FactoryAP","secret"
AT+CIFSR
AT+CIPMUX=0
AT+CIPSTART="TCP","192.168.1.50",9000
AT+CIPSEND=5
Hello
AT+CIPCLOSE
```

### B. SoftAP + TCP-сервер

```
AT+CWMODE=2
AT+CWSAP="DeviceAP","12345678",6,3
AT+CIPMUX=1
AT+CIPSERVER=1,8080
```

### C. HTTP GET / POST

```
AT+CWMODE=1
AT+CWJAP="FactoryAP","secret"
AT+HTTPCGET="http://192.168.1.20/api/status"
AT+HTTPCPOST="http://192.168.1.20/api/data",11
{"v":123}
```

### D. HTTPS GET

```
AT+HTTPCSNI="example.com"
AT+HTTPCGET="https://example.com/"
```

### E. mDNS / Bonjour

```
AT+CWMODE=1
AT+CWJAP="FactoryAP","secret"
AT+MDNS=1,"device","_http",80
```

### F. TLS-сокет с PIC-протоколом поверх SSL

```
AT+CIPSSLCSNI=0,"device.example.com"
AT+CIPSSLCCONF=0,2
AT+CIPSTART="SSL","device.example.com",443
AT+CIPSEND=...
```

---

## 8. Команды, которых нет в этой прошивке

Не регистрируются / не использовать:

| Группа | Примеры |
|---|---|
| MQTT | `AT+MQTTUSERCFG`, `AT+MQTTCONN`, `AT+MQTTPUB`… |
| WebSocket | `AT+WSHEAD`, `AT+WSOPEN`, `AT+WSSEND`… |
| BLE / BluFi | `AT+BLEINIT`, `AT+BLUFI`… |
| EAP | `AT+CWJEAP` |
| FS | `AT+FS...` |
| WebServer | `AT+WEBSERVER` |
| Wi-Fi OTA | `AT+CIUPDATE` |
| Driver AT | GPIO/I2C driver commands |

Их можно реализовать на PIC через `CIPSTART`/`CIPSEND` либо включить позже отдельной пересборкой ESP-AT.

---

## 9. Полезные ссылки Espressif

- Index: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/index.html
- Basic: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/Basic_AT_Commands.html
- Wi-Fi: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/Wi-Fi_AT_Commands.html
- TCP-IP: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/TCP-IP_AT_Commands.html
- HTTP: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/HTTP_AT_Commands.html
- User: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/AT_Command_Set/user_at_commands.html
- SPI AT host: https://docs.espressif.com/projects/esp-at/en/latest/esp32c2/Compile_and_Develop/How_to_implement_SPI_AT.html
