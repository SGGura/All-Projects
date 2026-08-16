# Simple demo application (GUI_GSP)

Minimal example that uses GUI_GSP: one screen, two labels, and a **language file** (JSON). Strings are taken from the language pack when available.

## Contents

- **main.c** — init, one screen, two labels (title and bottom line), main loop with `GUI_Run()`.
- **Langs.h** / **Langs.c** — key definitions for the language pack (AppTitle, Hello, BottomLine).
- **Langs_Pack/Langs/EN/English.json** — English strings for these keys (with font name per block). This example uses the **C font** **lv_font_unscii_8** (from `Fonts/lv_font_unscii_8.c` in the library). **No .bin file is needed** for that name — the library resolves it at runtime. For other font names (e.g. `"Ithaca_16"`) you must place the corresponding **.bin** in the same folder as the JSON.

## How to integrate into your project

1. Add the **GUI_GSP** library and your display driver (see [PORTING_GUIDE.md](../../PORTING_GUIDE.md)).
2. In your project, include **Langs.h** and **Langs.c** from this example (or copy and extend).
3. Copy **Langs_Pack/Langs** into your project and build the language pack (see [MEDIA_AND_LANG_PACK.md](../../MEDIA_AND_LANG_PACK.md)):
   - Run **CrateLangsPack.exe** on the Langs folder to produce **PackLang.lpn**.
   - Include **PackLang.lpn** in your FLASH media (e.g. via **CrateMedia.cmd** / grc.jar) so that **PackLang** is available.
4. In your firmware: call **Init_Langs((void *)&PackLang, 0)** after **GUI_GSP_Init()** if you have a language pack; otherwise the demo uses fallback strings.
5. **Main loop:** call only **GUI_Run()**.
6. **Timer interrupt** (e.g. every 10 ms): call **GUI_TimerClock(10)**.

## Language file format (English.json)

Each key corresponds to a **KEY_NAME_** in Langs.c. The value is an array: first elements are the strings (index 0, 1, …), the last element is the font name (e.g. `"lv_font_unscii_8"` for the built-in C font, or `"Ithaca_16"` for a FLASH font). For **C fonts** like **lv_font_unscii_8** (see `Fonts/lv_font_unscii_8.c`) **no .bin file is needed** — the library resolves the name to the built-in font. For fonts loaded from the pack (e.g. `"Ithaca_16"`) you must place **Ithaca_16.bin** in the same folder as the JSON. The demo uses **Set_Text_Font_Label()** so that both the string and the font from the pack (including C fonts) are applied.

## Building the language pack

From your project root (where CrateLangsPack and CrateMedia are configured):

1. Build **PackLang.lpn** using **CrateLangsPack.exe** from the repo ([Tools/CrateLangsPack.exe](../../Tools/CrateLangsPack.exe)):  
   `Tools\CrateLangsPack.exe "path\to\Langs_Pack\Langs" "path\to\Langs_Pack\PackLang.lpn" SMALL`
2. Add **PackLang.lpn** to your media script (e.g. CrateMedia.cmd) so it is embedded as **PackLang** in the FLASH binary.

Without a linked **PackLang**, the demo still runs and uses the fallback strings defined in main.c.
