# GUI_GSP Porting Guide

**Русская версия:** [PORTING_GUIDE.md](PORTING_GUIDE.md) · **Українська:** [PORTING_GUIDE_UK.md](PORTING_GUIDE_UK.md)

---

**A GUI library for small displays with a single framebuffer.**  
Step-by-step guide for developers: what GUI_GSP is, its features, how to port to your display, and how to use [LVGL](https://lvgl.io/) fonts.

---

## Contents

1. [About the library: small display, single buffer](#1-about-the-library-small-display-single-buffer)
2. [Library capabilities](#2-library-capabilities)
3. [LVGL font compatibility](#3-lvgl-font-compatibility)
4. [Who this guide is for](#4-who-this-guide-is-for)
5. [Porting in a nutshell](#5-porting-in-a-nutshell)
6. [Quick start](#6-quick-start)
7. [Porting steps in detail](#7-porting-steps-in-detail)
8. [Framebuffer format](#8-framebuffer-format)
9. [Additional features](#9-additional-features)
10. [Details: widgets, memory, timer](#10-details-widgets-memory-timer)
11. [Checklist and key files](#11-checklist-and-key-files)
12. [Using the library in your code](#12-using-the-library-in-your-code)
13. [FAQ](#13-faq)

---

## 1. About the library: small display, single buffer

**GUI_GSP** targets **embedded systems with small displays** (e.g. 128×64, 96×64, 128×32 pixels) and a **single framebuffer**.

- **Small display** — typical monochrome or simple color panels on SSD1306, SH1107 and similar controllers, used in instruments, panels, meters.
- **Single framebuffer** — the library keeps **one** pixel buffer (`Mem_LCD`). All UI is drawn into this buffer; then the buffer is sent to the display. Double buffering is not used: this reduces RAM use and simplifies porting to resource-limited MCUs (including PIC).

So GUI_GSP is optimized for limited memory and a simple loop: “draw frame into buffer → output buffer to display”.

---

## 2. Library capabilities

### 2.1. Graphics and display

- **Single shared framebuffer** — monochrome 1 bpp (one bit per pixel). Buffer size: `(SCREEN_HOR_SIZE × SCREEN_VER_SIZE) / 8` bytes.
- **All drawing inside the library** — pixels, lines, rectangles, fill, bitmap output. Application code does not implement low-level graphics.
- **Screens** — switch between “screens” (windows); each has its own widgets and key handler.
- **Safe screen switching** — `GUI_SwitchScreen(create_func)` and `GUI_IsScreenSwitchPending()` to change screens without touching freed widgets.

### 2.2. Widgets

| Widget | Purpose |
|--------|---------|
| **Screen** | Screen container: position, size, background color, border, widget list, key handler. |
| **Label** | Text with alignment, attributes (blink, shift, enable/disable), ticker (cyclic/infinite). |
| **Table** | Table of cells based on Label. |
| **List** | List of rows with scroll and ticker. |
| **Chart** | Charts: line, histogram, data shift (config options). |
| **Ruler** | Ruler (scale). |
| **Image** | Images from code (arrays) or external memory (FLASH). **Animation** support: sequence of frames with interval (ms), modes — loop, once, ping-pong; ImageSetFrames / ImageSetFramesEx, pause/resume. |
| **Line (PolyLine)** | Line primitive. |
| **ProgressBar** | Progress bar. |
| **Spinner** | Spinner (wait indicator). |
| **Panel** | Panel (container/frame). |
| **Timer** | **GSP_Timer** widget: timers tied to a screen — periodic callback (CreateTimer, period_ms, count). Used for periodic tasks and to drive animations. |
| **Message** | Message dialogs. |
| **Keyboard** | On-screen keyboard widget. |

Widget set is configured in the config file (`USED_LABEL`, `USED_TABLE`, `USED_CHART`, etc.); unused ones can be disabled to save code size.

### 2.3. Text and fonts

- **UTF-8 text output** — via `DrawTextUTF8` and related functions (`GSP_DrawText.c`).
- **[LVGL](https://lvgl.io/) font format** — `lv_font_t` structure, lv_font_fmt_txt format. You can use fonts built into the firmware or loaded from FLASH/external memory. See section 3.
- **Default font** — set by `DEFAULT_FONT` in `GUI_GSP_Config.h`; widgets can override with an explicit font.

### 2.4. Localization and external data

- **Language packs** — switch language, UI strings, fonts per language (including font loading from FLASH via `lv_font_load`). When building a pack, **next to each .json** you must place the converted LVGL font files used in the pack, with extension **.bin** (see MEDIA_AND_LANG_PACK).
- **External memory** — single callback `ExternalMemoryCallback` to read fonts and images from FLASH or external SPI-FLASH (options `USE_FONT_FLASH`, `USE_BITMAP_EXTERNAL`).

### 2.5. Input and timer

- **Keyboard** — poll keys with `GSP_KeyPoll()` and `GSP_ReadKey()`; bind to screen via `KeyHandler`.
- **GUI timer** — in the timer interrupt handler you must call **GUI_TimerClock(period_ms)** with the interrupt period in milliseconds (e.g. 10 ms).
- **Backlight** — implement `SetBright(light)` in your code if needed (PWM or GPIO).

In short: the library provides widgets, UTF-8 text with [LVGL](https://lvgl.io/) fonts, single-buffer operation, and minimal porting work — just init the display and output the buffer.

---

## 3. LVGL font compatibility

The library is **fully compatible with [LVGL](https://lvgl.io/) fonts**: same data structures and font format (lv_font_fmt_txt) as LVGL 8.x. You can use [LVGL tools](https://lvgl.io/tools) and existing fonts as-is.

### 3.1. Format and types

- **Font type:** `lv_font_t` (defined in `Fonts/LVGL/GSP_lvgl.h`).
- **Storage format:** **lv_font_fmt_txt** — glyph bitmaps, glyph descriptors (`lv_font_fmt_txt_glyph_dsc_t`), character maps (cmap), optional kerning. Structures match [LVGL Font Converter](https://lvgl.io/tools/fontconverter).
- **Bpp:** Fonts with 1, 2, 3, 4, 8 bpp are supported (including 1 bpp monochrome for small displays).

So any font generated for [LVGL](https://lvgl.io/) (e.g. with [LVGL Font Converter](https://lvgl.io/tools/fontconverter)) can be used in GUI_GSP without change.

### 3.2. Built-in fonts (in firmware)

The library supports **two font sources**:

1. **C fonts** — converted to `.c` (e.g. **lv_font_unscii_8.c**): add the file to the project, declare in **Fonts.h**, use `&lv_font_unscii_8` in code. In the language pack JSON you can use the name `"lv_font_unscii_8"` — **no .bin file next to the JSON is needed**; the library resolves it to the built-in font.
2. **FLASH fonts** — converted to **Binary** (`.bin`), placed next to `.json` and loaded via **lv_font_load** from the pack (see section 3.4 and MEDIA_AND_LANG_PACK).

**`GUI_GSP/Fonts/Fonts.h`** declares fonts included as source (`.c`) in the build:

| Font | Description |
|------|-------------|
| **lv_font_unscii_8** | Basic 8×8, ASCII (often used as default). |
| **lv_font_unscii_16** | Larger unscii. |
| **font_5x7_16**, **Pocket_Mem_16**, **Ithaca_16** | Additional fonts. |
| **AllLangFont** | Pointer to font for language names (may be loaded from FLASH). |

Widgets (Label, List, Table, Message, Keyboard, etc.) take an `lv_font_t *` (e.g. `&lv_font_unscii_8` or a FLASH-loaded font).

### 3.3. Default font (DEFAULT_FONT)

In **`GUI_GSP/Config/GUI_GSP_Config.h`**:

```c
#define DEFAULT_FONT  &lv_font_unscii_8
```

You can change it to any other built-in font from **Fonts.h** (e.g. `&lv_font_unscii_16`). If a widget does not set a font, `DEFAULT_FONT` is used.

### 3.4. Loading fonts from FLASH (external memory)

With **USE_FONT_FLASH** enabled and **GSP_HW.memory_read** set, the library can **load fonts at runtime** from FLASH or external memory:

- **lv_font_load()** (in `GSP_lv_font_loader.c`) reads the binary font by address via **ExternalMemoryCallback** / **memory_read**.
- Returns `lv_font_t *` for use in widgets or the language system.
- When done, call **lv_font_free(font)**.

In language packs, when switching language (**Load_Langs**), fonts can be loaded from the pack (FLASH) via **lv_font_load** and stored in **Ext_Fonts[]** and **AllLangFont**.

### 3.5. Getting fonts in LVGL format

- **[LVGL Font Converter](https://lvgl.io/tools/fontconverter)** ([LVGL](https://lvgl.io/)): load TTF/WOFF, set size, character range (e.g. Cyrillic), bpp; output:
  - **C array** — `.c` file (as **lv_font_unscii_8.c** in the repo): add to the project, declare in **Fonts.h**; use `&lv_font_unscii_8` in code; in the language pack JSON you can use the name `"lv_font_unscii_8"` — **no .bin needed**. Add more C fonts to the built-in table in **GSP_Langs.c** (`builtin_fonts` array).
  - **Binary** — `.bin` file for external memory, loaded with **lv_font_load**; place next to `.json` when building the pack (see MEDIA_AND_LANG_PACK).
- **Data for FLASH** (fonts, images, resources) can be prepared with [Microchip](https://www.microchip.com/) **grc.jar** (Graphics Resource Converter): part of Microchip tooling, converts fonts and images for internal/external memory. Run: `grc.jar` (Linux/macOS) or `launch_grc.bat` (Windows). Docs: [Microchip Developer Help — Graphics](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/mgs-harmony-guide/gfx-assets/).
- Generated fonts are **drop-in** compatible with GUI_GSP: same `lv_font_t` and lv_font_fmt_txt format.

### 3.6. UTF-8 and text output

Text is drawn via **DrawTextUTF8** and related functions (**GSP_DrawText.c**) with **UTF-8** support. Font is passed as `lv_font_t *font`. For Cyrillic or other scripts, include the right character range when converting the font.

**Summary for [LVGL](https://lvgl.io/):** the library uses the same fonts and format as LVGL 8.x. You can use **C fonts** (`.c` in the project, e.g. **lv_font_unscii_8.c**), **FLASH fonts** (`.bin` in the pack), or both; default font is **DEFAULT_FONT** in the config.

---

## 4. Who this guide is for

- You are porting a GUI_GSP project to **another MCU** or **another display**.
- You are adding GUI_GSP to a **new project**.
- You want to know **what you must implement** on your side.

**You do not need** to implement low-level graphics (pixels, lines, rectangles, images) — the library does that. You only **init the display** and **output the finished frame** to the screen.

---

## 5. Porting in a nutshell

The library keeps **one framebuffer** and draws widgets, text, and graphics into it. You provide **two functions**:

| # | Your function | Role |
|---|---------------|------|
| 1 | **Init_LCD()** | Once at startup: configure the display (pins, SPI/I2C, controller commands). |
| 2 | **Refresh_LCD(buffer)** | When the library calls you: send the framebuffer to the physical display. |

Flow:

```
  Your app (screens, buttons)
           │
           ▼
  GUI_GSP (widgets, drawing into Mem_LCD[])
           │
           ▼
  GSP_Display_Refresh()  ──►  calls your Refresh_LCD(Mem_LCD)
           │
           ▼
  Your code: send buffer via SPI/I2C/parallel to display
```

**Important:** You do **not** implement PutPixel, Line, Bar, SetColor, etc. — they are in the library and use its single buffer.

---

## 6. Quick start

1. **Create** a display module (e.g. `Graphics/MyDisplay.c` and `MyDisplay.h`).
2. **Implement**:
   - `void Init_LCD(void)` — hardware init.
   - `void Refresh_LCD(unsigned char *buffer)` — send `buffer` to the display (size/format in section 8).
3. **Set** in `GUI_GSP/Config/GUI_GSP_Config.h`: `SCREEN_HOR_SIZE`, `SCREEN_VER_SIZE`.
4. **In** `GUI_GSP/Drivers/GSP_Hardware.c`:
   - include your header with `Init_LCD` and `Refresh_LCD`;
   - in **GSP_HW** `.display`: **Refresh** = **GSP_Display_Refresh**, **Init** = **Init_LCD**.
5. **Add** your `MyDisplay.c` to the project build.

Details in section 7.

---

## 7. Porting steps in detail

### Step 1. Display module

- **Header** (e.g. `Graphics/MyDisplay.h`):

```c
#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

void Init_LCD(void);
void Refresh_LCD(unsigned char *buffer);

#endif
```

- **Implementation** (e.g. `Graphics/MyDisplay.c`): your init (GPIO, SPI, controller commands) and buffer output. Stub:

```c
#include "MyDisplay.h"

void Init_LCD(void)
{
    // Configure pins (CS, DC, RST, CLK, DATA…)
    // Configure SPI or other interface
    // Send controller init command sequence
}

void Refresh_LCD(unsigned char *buffer)
{
    // buffer size: (SCREEN_HOR_SIZE * SCREEN_VER_SIZE) / 8 bytes
    // Send to display: page-by-page or full — as your controller expects
}
```

Buffer size and format: section 8.

---

### Step 2. Set display dimensions

In **`GUI_GSP/Config/GUI_GSP_Config.h`**:

```c
#define SCREEN_HOR_SIZE    128   // width in pixels
#define SCREEN_VER_SIZE    64    // height in pixels
```

Buffer size: **SIZE_DISP = (SCREEN_HOR_SIZE × SCREEN_VER_SIZE) / 8** bytes (e.g. 128×64 → 1024 bytes).

---

### Step 3. Wire display in GSP_Hardware

In **`GUI_GSP/Drivers/GSP_Hardware.c`**:

1. Include your header with `Init_LCD` and `Refresh_LCD`.
2. In **GSP_HW** `.display`:
   - **Refresh** = **GSP_Display_Refresh** (library will call your **Refresh_LCD(Mem_LCD)**);
   - **Init** = **Init_LCD**.

Other fields (keyboard, stream_reader, memory_read, alloc/free) as needed (section 9).

---

### Step 4. Add files to the project

Add your display module (e.g. `MyDisplay.c`) to the build. If replacing an old display driver, remove the old module from the build.

---

## 8. Framebuffer format

**Refresh_LCD(unsigned char *buffer)** receives the buffer with the drawn frame.

- **Size:** `SIZE_DISP = (SCREEN_HOR_SIZE * SCREEN_VER_SIZE) / 8` bytes.
- **Layout (1 bpp monochrome):** one byte = 8 vertical pixels (page of 8 rows). Pixels by column: column 0 rows 0–7, then column 1 rows 0–7, etc.

**Byte index:** `index = (y / 8) * SCREEN_HOR_SIZE + x` (x = column, y = row).  
**Bits in byte:** LSB (bit 0) = smaller y in the 8-pixel group, MSB (bit 7) = larger y.

**Pixel meaning:** bit = 1 → light, bit = 0 → dark. Many controllers (SSD1306, SH1107) use this format; otherwise convert inside your **Refresh_LCD**.

---

## 9. Additional features

In **GSP_Hardware.c** you can also configure:

- **Keyboard** — `.keyboard.key_poll`, `.keyboard.read_key`; in the app use **GSP_KeyPoll()**, **GSP_ReadKey()**.
- **Backlight** — implement **SetBright(light)** in your module (or a stub).
- **External memory** — **memory_read** (read by offset and length), **stream_reader** (streaming read for JSON, etc.).
- **Allocator** — **GSP_HW.alloc**, **GSP_HW.free** (default: malloc/free or e.g. **user_malloc/user_free** from **GSP_mem_monitor**).

Unused pointers can be **NULL**; the library checks before calling.

---

## 10. Details: widgets, memory, timer

### 10.1. Widget and color config

In **GUI_GSP/Config/GUI_GSP_Config.h**: **USED_*** macros (USED_LABEL, USED_TABLE, USED_LIST, USED_CHART, USED_IMAGE, USED_PROGRESSBAR, USED_SPINNER, USED_PANEL, USED_LINE, USED_TIMER, USED_MESSAGE, USED_KEYBOARD). Disable unused widgets to reduce code size. Same file: default colors (DEFAULT_COLOR_*) and chart options (CHART_ENABLE_*).

### 10.2. External memory: fonts and images

Data from FLASH/external memory is read via **ExternalMemoryCallback** (signature in config), assigned to **GSP_HW.memory_read**. Used by the font loader (**lv_font_load**) when **USE_FONT_FLASH** and by image output (**PutImage**) when **USE_BITMAP_EXTERNAL**.

### 10.3. Font format in external memory

Font in external memory must be in **[LVGL](https://lvgl.io/) binary format** (export from [LVGL Font Converter](https://lvgl.io/tools/fontconverter) as **Binary**). Layout: head, cmap, loca, glyph (and optionally kern). **lv_font_load** reads them via **ExternalMemoryCallback**; **EXTDATA.address** must point to the full LVGL binary font file. Data to write to FLASH (fonts, images) can be prepared with [Microchip](https://www.microchip.com/) **grc.jar** (Graphics Resource Converter); see [Microchip graphics resources docs](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/mgs-harmony-guide/gfx-assets/).

### 10.4. Image format in external memory

With **USE_BITMAP_EXTERNAL**: at **EXTDATA.address** — header (compression, colorDepth, height, width), palette (two unsigned short for 1 bpp), then row-wise 1 bpp data. Read via **ExternalMemoryCallback**.

### 10.5. GSP_Timer widget

With **USED_TIMER** enabled, the **GSP_Timer** widget is available — timers tied to a screen. They call a user callback at a set interval and are useful for updating data, polling sensors, or syncing with animation.

- **CreateTimer(Screen, TimerFuncPtr, period_ms, count)** — create a timer: call `TimerFuncPtr(timer)` every `period_ms` ms; `count` is the number of firings (−1 = infinite).
- **DeleteAllTimers(Screen)**, **PauseAllTimers(Screen)**, **ResumeAllTimers(Screen)** — delete all timers of a screen, pause or resume.
- **Set_GSP_Timer_Period(Timer, period)** — change the timer period.

Timers are processed inside **GUI_Run()** using the global **Timer_GUI** counter, which is updated by **GUI_TimerClock(period_ms)** in the timer interrupt handler (see 10.7).

### 10.6. Image animation

The **Image** widget supports **animation** — sequential display of frames (array of bitmaps) with a given interval. Timing is based on **Timer_GUI**, so **GUI_TimerClock(period_ms)** must be called in the timer interrupt for animation to run.

- **Animation modes** (GSP_Image.h): **GSP_ANIM_MODE_LOOP** (loop), **GSP_ANIM_MODE_ONCE** (once then stop), **GSP_ANIM_MODE_PINGPONG** (back and forth), **GSP_ANIM_MODE_REVERSE** (reverse loop).
- **ImageSetFrames(img, frames, n, time)** — set frame array `frames`, number of frames `n`, interval between frames `time` (ms).
- **ImageSetFramesEx(img, frames, n, time, mode)** — same with explicit animation mode.
- **ImageStartAnimation(img)**, **ImageStopAnimation(img)**, **ImagePauseAnimation(img)**, **ImageResumeAnimation(img)**, **ImageRestartAnimation(img)** — control playback.
- **ImageSetAnimationMode(img, mode)** — change mode; **ImageGetCurrentFrame(img)**, **ImageIsAnimationPlaying(img)** — animation state.

Image blink: **ImageSetBlink(img, blink_time)** (0 to disable).

### 10.7. Timer and main loop

- **In the main loop** call **only** **GUI_Run()** — it handles keyboard polling, display refresh, and all GUI logic internally.
- **In the timer interrupt handler** you must call **GUI_TimerClock(period_ms)** with the interrupt period in milliseconds (e.g. for a 10 ms period call **GUI_TimerClock(10)**). This is required for **GSP_Timer** widgets, Image animation, and other features that use Timer_GUI.

Example: main loop and timer handler:

```c
// Main loop — only GUI_Run()
while (1) {
    GUI_Run();
}

// In timer interrupt handler (e.g. 10 ms period):
GUI_TimerClock(10);   // 10 = interrupt period in ms
```

---

## 11. Checklist and key files

### Porting checklist

- [ ] Display module with **Init_LCD()** and **Refresh_LCD(unsigned char *buffer)**.
- [ ] **SCREEN_HOR_SIZE** and **SCREEN_VER_SIZE** set in config.
- [ ] In **GSP_Hardware.c**: header included, **GSP_HW.display.Refresh = GSP_Display_Refresh**, **GSP_HW.display.Init = Init_LCD**.
- [ ] If needed: keyboard, stream_reader, memory_read, alloc/free, SetBright.
- [ ] Display module added to the build.

### Key files

| File | Purpose |
|------|---------|
| **GUI_GSP/Drivers/GSP_Driver.h**, **GSP_Driver.c** | Mem_LCD buffer, graphics, Refresh_LCD and Init_LCD calls. |
| **GUI_GSP/Drivers/GSP_Hardware.c** | GSP_HW setup (display, keyboard, memory, allocator). |
| **GUI_GSP/Config/GUI_GSP_Config.h** | Display size, colors, fonts, widget options. |
| **GUI_GSP/Fonts/Fonts.h**, **Fonts/LVGL/GSP_lvgl.h** | [LVGL](https://lvgl.io/) fonts, lv_font_t type. |
| **GUI_GSP/Fonts/LVGL/GSP_lv_font_loader.c** | lv_font_load / lv_font_free for loading fonts from FLASH. |

---

## 12. Using the library in your code

```c
#include "GUI_GSP/GUI_GSP.h"
#include "Langs.h"
```

**GUI_GSP.h** pulls in config, driver, and widgets.

---

## 13. FAQ

**Do I need to implement PutPixel, Line, Bar?**  
No. All graphics are in the library, drawing into one buffer. You only need Init_LCD and Refresh_LCD.

**What is passed to Refresh_LCD?**  
Pointer to the library buffer (Mem_LCD). Size: **(SCREEN_HOR_SIZE × SCREEN_VER_SIZE) / 8** bytes. Format in section 8.

**Can I use a different display size?**  
Yes. Set **SCREEN_HOR_SIZE** and **SCREEN_VER_SIZE** in the config.

**I have no keyboard / stream reader / external memory.**  
Leave the corresponding fields in GSP_HW as NULL.

**My display controller expects different page or bit order.**  
Do the conversion inside your **Refresh_LCD**.

**Can I use LVGL fonts?**  
Yes. The library is fully compatible with [LVGL](https://lvgl.io/) fonts (lv_font_t, lv_font_fmt_txt): built-in from **Fonts.h**, **DEFAULT_FONT** in config, and with **USE_FONT_FLASH** — load from FLASH via **lv_font_load**. See section 3.

---

*End of guide.*
