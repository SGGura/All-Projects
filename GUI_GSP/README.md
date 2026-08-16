# GUI_GSP

A GUI library for **embedded systems with small displays and a single framebuffer**. Optimized for limited RAM: one shared buffer, no double buffering.

**Features:** widgets (Screen, Label, Table, List, Chart, Image, ProgressBar, Spinner, Panel, **GSP_Timer**, Message, Keyboard), **[LVGL](https://lvgl.io/) fonts**, UTF-8 text output, **image animation** (Image: frames, loop/once/ping-pong modes), localization. Porting: implement display init and buffer output to the screen.

**Version:** 1.2 (see `GUI_GSP.h`)

---

## Integration

- Main header: `GUI_GSP/GUI_GSP.h`
- Config: `Config/GUI_GSP_Config.h`
- Languages/strings: `GSP_Langs.h`, `GSP_Langs.c`

In the main loop call only **GUI_Run()**; in the timer interrupt handler call **GUI_TimerClock(period_ms)** (period in ms, e.g. 10).

---

## Documentation

Detailed porting guide, capabilities, and LVGL fonts:

- **[PORTING_GUIDE.md](PORTING_GUIDE.md)** (Russian)
- **[PORTING_GUIDE_EN.md](PORTING_GUIDE_EN.md)** (English)
- **[PORTING_GUIDE_UK.md](PORTING_GUIDE_UK.md)** (Ukrainian)
- **[MEDIA_AND_LANG_PACK.md](MEDIA_AND_LANG_PACK.md)** (EN) / **[MEDIA_AND_LANG_PACK_RU.md](MEDIA_AND_LANG_PACK_RU.md)** (RU) / **[MEDIA_AND_LANG_PACK_UK.md](MEDIA_AND_LANG_PACK_UK.md)** (UK) — creating FLASH binary (CrateMedia.cmd) and language pack (Langs.c, English.json → PackLang.lpn). Fonts can be **C sources** (e.g. **lv_font_unscii_8.c** in the project; no .bin needed) or **Binary** (**.bin** next to each .json for FLASH). **[Tools/](Tools/)** — **CrateLangsPack.exe** (included in repo) and [Tools/README.md](Tools/README.md).

---

## Structure

- `Config/` — display size, colors, widget options, default font
- `Drivers/` — framebuffer, graphics, display binding (Init_LCD, Refresh_LCD)
- `Fonts/` — LVGL-format fonts (built-in and load from FLASH)
- Widgets: Label, Chart, Table, List, Panel, Image (incl. animation), **GSP_Timer**, Message, Keyboard, etc.

---

## Example

**[Examples/simple_demo](Examples/simple_demo)** — minimal app: one screen, two labels, and a **language file** (JSON). Shows how to use Langs.h/Langs.c, English.json, and (optionally) the FLASH language pack with `Init_Langs` and `Get_String_From_LangPack`.

---

## License

Use under the terms of your project. When using in public repositories, please credit this repository (link to source).
