# ESP8684-MINI-1-H2X — ESP-AT SPI build

Module name: **ESP32C2-MINI-1-H2X-SPI**  
Config: `module_config/module_esp32c2-mini-1-h2x-spi/`

## Pins (SPI AT, standard mode)

| Signal | GPIO |
|--------|------|
| SCLK | 6 |
| MOSI | 7 |
| MISO | 2 |
| CS | 10 |
| HANDSHAKE | 3 |
| Console UART TX/RX | 8 / 9 (logs only) |

## Build

```bat
cd D:\MCS_ESP32\SPI_WiFi
python build.py install
python build.py build
```

`build/module_info.json` already selects this module.

Firmware output: `build/esp-at.bin` and `build/download.config`.

Flash size: **2 MB** (H2X).

## Enabled HTTP client

The firmware supports HTTP/HTTPS client AT commands, including GET and POST.
The HTTP TX and RX buffers are 2048 bytes each. Wi-Fi OTA remains disabled.

## Produced build

The configured build uses an `ota_0` app partition of `0x130000` bytes
(1,245,184 bytes). The SPI AT application is 1,061,472 bytes
(HTTP client enabled, OTA off), leaving 183,712 bytes free.

Flash the complete production image:

```bat
cd D:\MCS_ESP32\SPI_WiFi
C:\Users\gura0\.espressif\python_env\idf6.1_py3.14_env\Scripts\python.exe -m esptool --chip esp32c2 --port COMx --baud 460800 write-flash --flash-mode dio --flash-freq 60m --flash-size 2MB 0x0 build\factory\factory_ESP32C2-MINI-1-H2X-SPI.bin
```

## ESP-IDF

The local `esp-idf/` tree is not stored in this repository (too large).
On a build machine run:

```bat
python build.py install
python build.py build
```

Or point `IDF_PATH` to an already installed ESP-IDF matching `module_config/module_esp32c2-mini-1-h2x-spi/IDF_VERSION`.

Documentation: `DOC/FIRMWARE_CAPABILITIES.md`, `DOC/AT_COMMANDS.md`.
