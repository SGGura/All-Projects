# Creating FLASH data and language packs

**Русская версия:** [MEDIA_AND_LANG_PACK_RU.md](MEDIA_AND_LANG_PACK_RU.md) · **Українська:** [MEDIA_AND_LANG_PACK_UK.md](MEDIA_AND_LANG_PACK_UK.md)

---

This document describes how to generate the binary file for FLASH (images + language pack) and how to prepare a language pack from JSON and project keys (`Langs.c`), using the **CrateMedia.cmd** and **CrateLangsPack** workflow as an example.

---

## 1. Creating the file for FLASH (CrateMedia.cmd)

The **CrateMedia.cmd** script uses [Microchip](https://www.microchip.com/) **grc.jar** (Graphics Resource Converter) to build a single binary file (e.g. `Media_UT_Base.bin`) that is later placed in external FLASH or linked for the target. That file can contain both **images** (BMP) and the **language pack** (`.lpn`).

### 1.1. What CrateMedia.cmd does

- **Inputs:**  
  - Images: e.g. `Media\Screen_Start.bmp`, `Media\Bat8.bmp`, …  
  - Language pack: `Langs_Pack\PackLang.lpn` (see section 2).
- **Output:** one binary file, e.g. `Media\Media_UT_Base.bin`.
- **Options** (example): `-T I` (Image), `-F B` (Binary), `-O "<out.bin>"`, `-B C32`. Each resource is added with `-I "<path>"` and `-X "<SymbolName>"`.

### 1.2. Example structure of CrateMedia.cmd

```batch
@echo off
set "GRC_JAR=D:\path\to\grc.jar"
set "MEDIA_DIR=D:\path\to\Source\Media"
set "LANG_DIR=D:\path\to\Source\Langs_Pack"
set "OUT_FILE=%MEDIA_DIR%\Media_UT_Base.bin"

rem Common options: Image, Binary, Output, Compiler
set "OPTS=-T I -F B -O "%OUT_FILE%" -B C32"

rem Add images
java.exe -jar "%GRC_JAR%" %OPTS% -I "%MEDIA_DIR%\Screen_Start.bmp" -X "Pic_StartScreen" ; ^
%OPTS% -I "%MEDIA_DIR%\Bat0.bmp" -X "Bat0" ; ^
...

rem Add language pack (built in step 2)
%OPTS% -I "%LANG_DIR%\PackLang.lpn" -X "PackLang"
```

Running **CrateMedia.cmd** produces the FLASH binary that the firmware (or external loader) uses. The symbol **PackLang** is the language pack blob; its base address/size are used when calling `Init_Langs((void *)&PackLang, Lang)` (see GSP_Langs).

---

## 2. Creating the language pack (example with Langs.c and English.json)

The language pack is built in two steps: (1) from JSON + fonts to **PackLang.lpn**, (2) then **PackLang.lpn** is included into the FLASH binary by CrateMedia.cmd (above).

**Important:** In the **same folder as each .json** file you must place the **converted LVGL font files used in that pack, with extension .bin** (e.g. **Ithaca_16.bin**). CrateLangsPack expects these `.bin` files next to the JSON; font names in the JSON (e.g. `"Ithaca_16"`) must match the file name without `.bin`.

### 2.1. Project keys (Langs.c / Langs.h)

In the application, each UI string group is represented by a **KEY_NAME_** whose `key_name` matches a key in the JSON file:

- **Langs.h** — declares `extern KEY_NAME_ MainMenuName;`, `extern KEY_NAME_ SettingsName;`, etc.
- **Langs.c** — defines them, e.g. `KEY_NAME_ MainMenuName = {"MainMenuName"};`

The string **"MainMenuName"** must match the key used in the JSON (e.g. in **English.json**). The library resolves keys via **Get_String_From_LangPack(Lang, Key, n)** and similar (see GSP_Langs.h).

### 2.2. JSON language file (e.g. English.json)

Each key maps to an array of strings and the font name for that block. Example layout:

```json
[
    {"NameLang": [ "English", "AllLangFont" ] },
    {"MainMenuName": [ "MAIN MENU", "Measurements", "Probes", "Archive", "Settings", "Info", "Ithaca_16" ] },
    {"MainMenuBottom": [ "< Power off", "Select >", "Ithaca_16" ] },
    {"SettingsName": [ "SETTINGS", "Alarm", "Alarm max", "Language", "Ithaca_16" ] }
]
```

- First elements of the array = the strings for that key (index 0, 1, 2, …).
- Last element = font name (e.g. **"Ithaca_16"**, **"AllLangFont"**) used for that block; the library loads these fonts from the pack when **Init_Langs** / **Load_Langs** is used.

Place the JSON file in a per-language folder, e.g. **Langs_Pack\Langs\EN\English.json**. **In the same folder as the JSON** you must put the **converted LVGL font binaries** (e.g. **Ithaca_16.bin**). Font names in the JSON (e.g. `"Ithaca_16"`) must match the font file names without the `.bin` extension. Generate the `.bin` with [LVGL Font Converter](https://lvgl.io/tools/fontconverter) (export as **Binary**).

**C fonts (no .bin needed):** If you use a font linked as C source (e.g. **lv_font_unscii_8.c**, declared in **Fonts.h**), you can put its name in the JSON (e.g. `"lv_font_unscii_8"`). **No .bin file is required** — the library resolves it to the built-in font at runtime. Only fonts loaded from the pack (FLASH) need a `.bin` next to the JSON. See [PORTING_GUIDE](PORTING_GUIDE_EN.md) section 3.2.

### 2.3. Building PackLang.lpn

**CrateLangsPack.exe** is included in this repository under **[Tools/CrateLangsPack.exe](Tools/CrateLangsPack.exe)**. It reads the **Langs** folder (one subfolder per language with JSON files) and writes **PackLang.lpn**:

```batch
set "LANG_SOURCE=D:\path\to\Source\Langs_Pack\Langs"
set "LPN_OUT=D:\path\to\Source\Langs_Pack\PackLang.lpn"
"path\to\GUI_GSP\Tools\CrateLangsPack.exe" "%LANG_SOURCE%" "%LPN_OUT%" SMALL
```

- **LANG_SOURCE** — folder with subfolders (e.g. EN, RU), each containing one or more JSON files (e.g. English.json).
- **LPN_OUT** — path to the generated **PackLang.lpn**.
- **SMALL** — option (tool-specific; may control format or size).

After this, run **CrateMedia.cmd** so that **PackLang.lpn** is included in the FLASH binary as **PackLang**.

### 2.4. Build flow for language pack and FLASH media

For the language pack and FLASH media used with GUI_GSP:

1. **CrateLangsPack** — build **PackLang.lpn** from `Langs_Pack\Langs` (input: folder with EN, RU, … and JSON + font `.bin` files; output: **PackLang.lpn**).
2. **CrateMedia.cmd** (optional) — grc.jar builds **Media_UT_Base.bin** for FLASH (images + **PackLang.lpn**).

See also [Tools/README.md](Tools/README.md).

### 2.5. Using the pack in firmware

- At startup, call **Init_Langs((void *)&PackLang, Config.Lang)** where **PackLang** is the language pack resource (from the FLASH binary built by CrateMedia.cmd).
- To switch language: **Load_Langs(new_lang)**.
- To get a string: **Get_String_From_LangPack(Lang, &MainMenuName, index)**.

---

## Summary

| Step | Tool / file | Input | Output |
|------|-------------|--------|--------|
| 1 | **Langs.c / Langs.h** | — | KEY_NAME_ with keys matching JSON |
| 2 | **English.json** (and other languages) | Strings + font names per key | In **Langs_Pack\Langs\EN\** (etc.) |
| 2b | **Fonts used in the pack** | LVGL fonts exported as **Binary** | **Next to each .json**: e.g. **Ithaca_16.bin** (same folder as the JSON) |
| 3 | **CrateLangsPack.exe** ([Tools/](Tools/)) | Langs_Pack\Langs (JSON + font `.bin` files) | **PackLang.lpn** |
| 4 | **CrateMedia.cmd** (grc.jar) | BMPs + **PackLang.lpn** | **Media_UT_Base.bin** (file for FLASH) |

The firmware uses the FLASH binary (e.g. via **PackLang** and **Init_Langs**) and gets strings/fonts from the language pack at runtime.
