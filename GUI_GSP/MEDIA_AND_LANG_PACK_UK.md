# Створення файлу для FLASH та мовних пакетів

У документі описано, як отримати бінарний файл для FLASH (зображення та мовний пакет) і як підготувати мовний пакет з JSON та ключів проекту (`Langs.c`), на прикладі сценарію **CrateMedia.cmd** та **CrateLangsPack**.

---

## 1. Створення файлу для FLASH (CrateMedia.cmd)

Скрипт **CrateMedia.cmd** використовує [Microchip](https://www.microchip.com/) **grc.jar** (Graphics Resource Converter) для збірки одного бінарного файлу (наприклад, `Media_UT_Base.bin`), який потім розміщується у зовнішній FLASH або лінкується в прошивку. У цей файл входять і **зображення** (BMP), і **мовний пакет** (`.lpn`).

### 1.1. Що робить CrateMedia.cmd

- **Входи:**  
  - Зображення: наприклад `Media\Screen_Start.bmp`, `Media\Bat8.bmp`, …  
  - Мовний пакет: `Langs_Pack\PackLang.lpn` (див. розділ 2).
- **Вихід:** один бінарний файл, наприклад `Media\Media_UT_Base.bin`.
- **Опції** (приклад): `-T I` (Image), `-F B` (Binary), `-O "<out.bin>"`, `-B C32`. Кожен ресурс додається ключами `-I "<шлях>"` та `-X "<ім'я_символу>"`.

### 1.2. Приклад структури CrateMedia.cmd

```batch
@echo off
set "GRC_JAR=D:\path\to\grc.jar"
set "MEDIA_DIR=D:\path\to\Source\Media"
set "LANG_DIR=D:\path\to\Source\Langs_Pack"
set "OUT_FILE=%MEDIA_DIR%\Media_UT_Base.bin"

rem Загальні опції: Image, Binary, вихідний файл, компілятор
set "OPTS=-T I -F B -O "%OUT_FILE%" -B C32"

rem Додавання картинок
java.exe -jar "%GRC_JAR%" %OPTS% -I "%MEDIA_DIR%\Screen_Start.bmp" -X "Pic_StartScreen" ; ^
%OPTS% -I "%MEDIA_DIR%\Bat0.bmp" -X "Bat0" ; ^
...

rem Додавання мовного пакета (зібраного в кроці 2)
%OPTS% -I "%LANG_DIR%\PackLang.lpn" -X "PackLang"
```

Після запуску **CrateMedia.cmd** виходить бінарник для FLASH, який використовує прошивка. Символ **PackLang** — це блок даних мовного пакета; його базова адреса та розмір використовуються при виклику `Init_Langs((void *)&PackLang, Lang)` (див. GSP_Langs).

---

## 2. Створення мовного пакета (приклад з Langs.c та English.json)

Мовний пакет збирається у два кроки: (1) з JSON та шрифтів у **PackLang.lpn**, (2) потім **PackLang.lpn** включається в бінарник для FLASH через CrateMedia.cmd (див. вище).

**Важливо:** у **тій самій папці, де лежить кожен .json**, мають лежати **використовувані в пакеті сконвертовані шрифти LVGL у вигляді файлів з розширенням .bin** (наприклад **Ithaca_16.bin**). CrateLangsPack очікує ці файли `.bin` поруч із JSON; імена шрифтів у JSON (наприклад `"Ithaca_16"`) мають збігатися з іменем файлу без `.bin`.

### 2.1. Ключі проекту (Langs.c / Langs.h)

У застосунку кожна група рядків інтерфейсу задається структурою **KEY_NAME_**, у якої `key_name` збігається з ключем у JSON:

- **Langs.h** — оголошення `extern KEY_NAME_ MainMenuName;`, `extern KEY_NAME_ SettingsName;` тощо.
- **Langs.c** — визначення, наприклад `KEY_NAME_ MainMenuName = {"MainMenuName"};`

Рядок **"MainMenuName"** має збігатися з ключем у JSON (наприклад, у **English.json**). Бібліотека знаходить рядки за ключами через **Get_String_From_LangPack(Lang, Key, n)** та аналоги (див. GSP_Langs.h).

### 2.2. JSON-файл мови (наприклад English.json)

Кожен ключ задає масив рядків та ім’я шрифту для цієї групи. Приклад структури:

```json
[
    {"NameLang": [ "English", "AllLangFont" ] },
    {"MainMenuName": [ "MAIN MENU", "Measurements", "Probes", "Archive", "Settings", "Info", "Ithaca_16" ] },
    {"MainMenuBottom": [ "< Power off", "Select >", "Ithaca_16" ] },
    {"SettingsName": [ "SETTINGS", "Alarm", "Alarm max", "Language", "Ithaca_16" ] }
]
```

- Перші елементи масиву — рядки для цього ключа (індекси 0, 1, 2, …).
- Останній елемент — ім’я шрифту (наприклад **"Ithaca_16"**, **"AllLangFont"**) для цієї групи; бібліотека підвантажує ці шрифти з пакета при **Init_Langs** / **Load_Langs**.

JSON розміщують у папці мови, наприклад **Langs_Pack\Langs\EN\English.json**. **У тій самій папці, що й JSON**, мають лежати **сконвертовані бінарники шрифтів LVGL** (наприклад **Ithaca_16.bin**). Імена шрифтів у JSON (наприклад `"Ithaca_16"`) мають збігатися з іменами файлів шрифтів без розширення `.bin`. Отримати `.bin` можна в [LVGL Font Converter](https://lvgl.io/tools/fontconverter) (експорт у **Binary**).

**Шрифти в C (без .bin):** якщо ви використовуєте шрифт, підключений як вихідник C (наприклад **lv_font_unscii_8.c**, оголошений у **Fonts.h**), у JSON можна вказати його ім'я (наприклад `"lv_font_unscii_8"`). **Файл .bin для такого імені не потрібен** — бібліотека підставить вбудований шрифт під час роботи. Файли `.bin` поруч із JSON потрібні лише для шрифтів, що завантажуються з пакета (FLASH). Див. [PORTING_GUIDE](PORTING_GUIDE_UK.md) розділ 3.2.

### 2.3. Збірка PackLang.lpn

**CrateLangsPack.exe** входить у цей репозиторій: **[Tools/CrateLangsPack.exe](Tools/CrateLangsPack.exe)**. Утиліта читає папку **Langs** (підпапки по мовах з JSON) і створює **PackLang.lpn**:

```batch
set "LANG_SOURCE=D:\path\to\Source\Langs_Pack\Langs"
set "LPN_OUT=D:\path\to\Source\Langs_Pack\PackLang.lpn"
"path\to\GUI_GSP\Tools\CrateLangsPack.exe" "%LANG_SOURCE%" "%LPN_OUT%" SMALL
```

- **LANG_SOURCE** — папка з підпапками (EN, RU тощо), у кожній — JSON-файли (наприклад English.json).
- **LPN_OUT** — шлях до створюваного **PackLang.lpn**.
- **SMALL** — опція (залежить від утиліти; може задавати формат або розмір).

Після цього запускають **CrateMedia.cmd**, щоб **PackLang.lpn** потрапив у бінарник для FLASH як **PackLang**.

### 2.4. Збірка мовного пакета та медіа для FLASH

Для мовного пакета та медіа FLASH, що використовуються з GUI_GSP:

1. **CrateLangsPack** — збірка **PackLang.lpn** з `Langs_Pack\Langs` (вхід: папка з EN, RU, … та JSON + файли шрифтів `.bin`; вихід: **PackLang.lpn**).
2. **CrateMedia.cmd** (за потреби) — grc.jar формує **Media_UT_Base.bin** для FLASH (зображення + **PackLang.lpn**).

Див. також [Tools/README.md](Tools/README.md).

### 2.5. Використання пакета в прошивці

- При старті викликають **Init_Langs((void *)&PackLang, Config.Lang)**, де **PackLang** — ресурс мовного пакета з бінарника, зібраного CrateMedia.cmd.
- Зміна мови: **Load_Langs(new_lang)**.
- Отримання рядка: **Get_String_From_LangPack(Lang, &MainMenuName, index)**.

---

## Підсумок

| Крок | Інструмент / файл | Вхід | Вихід |
|------|-------------------|------|--------|
| 1 | **Langs.c / Langs.h** | — | KEY_NAME_ з ключами, що збігаються з JSON |
| 2 | **English.json** (та ін. мови) | Рядки та імена шрифтів за ключами | У **Langs_Pack\Langs\EN\** тощо |
| 2б | **Шрифти, що використовуються в пакеті** | Шрифти LVGL, експорт у **Binary** | **Поруч із кожним .json**: наприклад **Ithaca_16.bin** (у тій самій папці, що й JSON) |
| 3 | **CrateLangsPack.exe** ([Tools/](Tools/)) | Langs_Pack\Langs (JSON + файли шрифтів `.bin`) | **PackLang.lpn** |
| 4 | **CrateMedia.cmd** (grc.jar) | BMP + **PackLang.lpn** | **Media_UT_Base.bin** (файл для FLASH) |

Прошивка використовує бінарник для FLASH (через **PackLang** та **Init_Langs**) і отримує рядки та шрифти з мовного пакета в runtime.

---

**Русская версия:** [MEDIA_AND_LANG_PACK_RU.md](MEDIA_AND_LANG_PACK_RU.md) · **English:** [MEDIA_AND_LANG_PACK.md](MEDIA_AND_LANG_PACK.md)
