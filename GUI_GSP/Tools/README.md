# Tools

## CrateLangsPack.exe

Utility to build the language pack file **PackLang.lpn** from JSON language files (e.g. `Langs_Pack\Langs\EN\English.json`). **In the same folder as each JSON** you must place the converted LVGL font binaries (e.g. **Ithaca_16.bin**); font names in the JSON must match the file names without the `.bin` extension.

**Usage:**

```batch
CrateLangsPack.exe "<LANG_SOURCE>" "<LPN_OUT>" SMALL
```

- **LANG_SOURCE** — path to the folder containing one subfolder per language (EN, RU, …), each with JSON files.
- **LPN_OUT** — output path for **PackLang.lpn**.
- **SMALL** — option (format/size).

**Example:** see [MEDIA_AND_LANG_PACK.md](../MEDIA_AND_LANG_PACK.md).

## Build flow for language pack and FLASH media

For the GUI_GSP library and examples, when using language packs and external FLASH media:

1. **CrateLangsPack** — build **PackLang.lpn** from `Langs_Pack\Langs`:
   - Input: e.g. `Source\Langs_Pack\Langs` (with EN, RU, … and JSON + font `.bin` files inside).
   - Output: `Source\Langs_Pack\PackLang.lpn`.
2. **CrateMedia.cmd** (optional, if media is built separately) — grc.jar adds images and **PackLang.lpn** into **Media_UT_Base.bin** for FLASH.
