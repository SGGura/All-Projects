# Создание файла для FLASH и языковых пакетов

В документе описано, как получить бинарный файл для FLASH (картинки и языковой пакет) и как подготовить языковой пакет из JSON и ключей проекта (`Langs.c`), на примере сценария **CrateMedia.cmd** и **CrateLangsPack**.

---

## 1. Создание файла для FLASH (CrateMedia.cmd)

Скрипт **CrateMedia.cmd** использует [Microchip](https://www.microchip.com/) **grc.jar** (Graphics Resource Converter) для сборки одного бинарного файла (например, `Media_UT_Base.bin`), который затем размещается во внешней FLASH или линкуется в прошивку. В этот файл входят и **изображения** (BMP), и **языковой пакет** (`.lpn`).

### 1.1. Что делает CrateMedia.cmd

- **Входы:**  
  - Изображения: например `Media\Screen_Start.bmp`, `Media\Bat8.bmp`, …  
  - Языковой пакет: `Langs_Pack\PackLang.lpn` (см. раздел 2).
- **Выход:** один бинарный файл, например `Media\Media_UT_Base.bin`.
- **Опции** (пример): `-T I` (Image), `-F B` (Binary), `-O "<out.bin>"`, `-B C32`. Каждый ресурс добавляется ключами `-I "<путь>"` и `-X "<имя_символа>"`.

### 1.2. Пример структуры CrateMedia.cmd

```batch
@echo off
set "GRC_JAR=D:\path\to\grc.jar"
set "MEDIA_DIR=D:\path\to\Source\Media"
set "LANG_DIR=D:\path\to\Source\Langs_Pack"
set "OUT_FILE=%MEDIA_DIR%\Media_UT_Base.bin"

rem Общие опции: Image, Binary, выходной файл, компилятор
set "OPTS=-T I -F B -O "%OUT_FILE%" -B C32"

rem Добавление картинок
java.exe -jar "%GRC_JAR%" %OPTS% -I "%MEDIA_DIR%\Screen_Start.bmp" -X "Pic_StartScreen" ; ^
%OPTS% -I "%MEDIA_DIR%\Bat0.bmp" -X "Bat0" ; ^
...

rem Добавление языкового пакета (собранного в шаге 2)
%OPTS% -I "%LANG_DIR%\PackLang.lpn" -X "PackLang"
```

После запуска **CrateMedia.cmd** получается бинарник для FLASH, который использует прошивка. Символ **PackLang** — это блок данных языкового пакета; его базовый адрес и размер используются при вызове `Init_Langs((void *)&PackLang, Lang)` (см. GSP_Langs).

---

## 2. Создание языкового пакета (пример с Langs.c и English.json)

Языковой пакет собирается в два шага: (1) из JSON и шрифтов в **PackLang.lpn**, (2) затем **PackLang.lpn** включается в бинарник для FLASH через CrateMedia.cmd (см. выше).

**Важно:** в **той же папке, где лежит каждый .json**, должны находиться **используемые в пакете сконвертированные шрифты LVGL в виде файлов с расширением .bin** (например **Ithaca_16.bin**). CrateLangsPack ожидает эти файлы `.bin` рядом с JSON; имена шрифтов в JSON (например `"Ithaca_16"`) должны совпадать с именем файла без `.bin`.

### 2.1. Ключи проекта (Langs.c / Langs.h)

В приложении каждая группа строк интерфейса задаётся структурой **KEY_NAME_**, у которой `key_name` совпадает с ключом в JSON:

- **Langs.h** — объявления `extern KEY_NAME_ MainMenuName;`, `extern KEY_NAME_ SettingsName;` и т.д.
- **Langs.c** — определения, например `KEY_NAME_ MainMenuName = {"MainMenuName"};`

Строка **"MainMenuName"** должна совпадать с ключом в JSON (например, в **English.json**). Библиотека находит строки по ключам через **Get_String_From_LangPack(Lang, Key, n)** и аналоги (см. GSP_Langs.h).

### 2.2. JSON-файл языка (например English.json)

Каждый ключ задаёт массив строк и имя шрифта для этой группы. Пример структуры:

```json
[
    {"NameLang": [ "English", "AllLangFont" ] },
    {"MainMenuName": [ "MAIN MENU", "Measurements", "Probes", "Archive", "Settings", "Info", "Ithaca_16" ] },
    {"MainMenuBottom": [ "< Power off", "Select >", "Ithaca_16" ] },
    {"SettingsName": [ "SETTINGS", "Alarm", "Alarm max", "Language", "Ithaca_16" ] }
]
```

- Первые элементы массива — строки для этого ключа (индексы 0, 1, 2, …).
- Последний элемент — имя шрифта (например **"Ithaca_16"**, **"AllLangFont"**) для этой группы; библиотека подгружает эти шрифты из пакета при **Init_Langs** / **Load_Langs**.

JSON размещают в папке языка, например **Langs_Pack\Langs\EN\English.json**. **В той же папке, что и JSON**, должны лежать **сконвертированные бинарники шрифтов LVGL** (например **Ithaca_16.bin**). Имена шрифтов в JSON (например `"Ithaca_16"`) должны совпадать с именами файлов шрифтов без расширения `.bin`. Получить `.bin` можно в [LVGL Font Converter](https://lvgl.io/tools/fontconverter) (экспорт в **Binary**).

**Шрифты в C (без .bin):** если вы используете шрифт, подключённый как исходник C (например **lv_font_unscii_8.c**, объявленный в **Fonts.h**), в JSON можно указать его имя (например `"lv_font_unscii_8"`). **Файл .bin для такого имени не нужен** — библиотека подставит встроенный шрифт при работе. Файлы `.bin` рядом с JSON нужны только для шрифтов, загружаемых из пакета (FLASH). См. [PORTING_GUIDE](PORTING_GUIDE.md) раздел 3.2.

### 2.3. Сборка PackLang.lpn

**CrateLangsPack.exe** входит в этот репозиторий: **[Tools/CrateLangsPack.exe](Tools/CrateLangsPack.exe)**. Утилита читает папку **Langs** (подпапки по языкам с JSON) и создаёт **PackLang.lpn**:

```batch
set "LANG_SOURCE=D:\path\to\Source\Langs_Pack\Langs"
set "LPN_OUT=D:\path\to\Source\Langs_Pack\PackLang.lpn"
"path\to\GUI_GSP\Tools\CrateLangsPack.exe" "%LANG_SOURCE%" "%LPN_OUT%" SMALL
```

- **LANG_SOURCE** — папка с подпапками (EN, RU и т.д.), в каждой — JSON-файлы (например English.json).
- **LPN_OUT** — путь к создаваемому **PackLang.lpn**.
- **SMALL** — опция (зависит от утилиты; может задавать формат или размер).

После этого запускают **CrateMedia.cmd**, чтобы **PackLang.lpn** попал в бинарник для FLASH как **PackLang**.

### 2.4. Сборка языкового пакета и медиа для FLASH

Для языкового пакета и медиа FLASH, используемых с GUI_GSP:

1. **CrateLangsPack** — сборка **PackLang.lpn** из `Langs_Pack\Langs` (вход: папка с EN, RU, … и JSON + файлы шрифтов `.bin`; выход: **PackLang.lpn**).
2. **CrateMedia.cmd** (при необходимости) — grc.jar формирует **Media_UT_Base.bin** для FLASH (картинки + **PackLang.lpn**).

См. также [Tools/README.md](Tools/README.md).

### 2.5. Использование пакета в прошивке

- При старте вызывают **Init_Langs((void *)&PackLang, Config.Lang)**, где **PackLang** — ресурс языкового пакета из бинарника, собранного CrateMedia.cmd.
- Смена языка: **Load_Langs(new_lang)**.
- Получение строки: **Get_String_From_LangPack(Lang, &MainMenuName, index)**.

---

## Итог

| Шаг | Инструмент / файл | Вход | Выход |
|-----|-------------------|------|--------|
| 1 | **Langs.c / Langs.h** | — | KEY_NAME_ с ключами, совпадающими с JSON |
| 2 | **English.json** (и др. языки) | Строки и имена шрифтов по ключам | В **Langs_Pack\Langs\EN\** и т.д. |
| 2б | **Шрифты, используемые в пакете** | Шрифты LVGL, экспорт в **Binary** | **Рядом с каждым .json**: например **Ithaca_16.bin** (в той же папке, что и JSON) |
| 3 | **CrateLangsPack.exe** ([Tools/](Tools/)) | Langs_Pack\Langs (JSON + файлы шрифтов `.bin`) | **PackLang.lpn** |
| 4 | **CrateMedia.cmd** (grc.jar) | BMP + **PackLang.lpn** | **Media_UT_Base.bin** (файл для FLASH) |

Прошивка использует бинарник для FLASH (через **PackLang** и **Init_Langs**) и получает строки и шрифты из языкового пакета в runtime.

---

**English:** [MEDIA_AND_LANG_PACK.md](MEDIA_AND_LANG_PACK.md) · **Українська:** [MEDIA_AND_LANG_PACK_UK.md](MEDIA_AND_LANG_PACK_UK.md)
