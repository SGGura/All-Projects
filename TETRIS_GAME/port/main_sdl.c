/*
 * main_sdl.c - desktop simulator for the 240x320 button-only build.
 *
 * The window is a pixel exact stand-in for the panel: the game renders into a
 * 240x320 RGB565 buffer that is uploaded as-is. Only the keyboard is read,
 * mouse and touch events are deliberately ignored so the simulator cannot do
 * anything the target hardware cannot.
 */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "tetris.h"
#include "bot.h"

#define HISCORE_FILE "tetris_hiscore.dat"

static uint16_t framebuffer[TETRIS_SCREEN_W * TETRIS_SCREEN_H];

/* Keyboard stands in for the six hardware buttons. Letters that name a
   hardware button are avoided on purpose, so nothing collides with the button
   list drawn on the title screen. */
static const struct {
    SDL_Scancode key;
    uint32_t     button;
} keymap[] = {
    { SDL_SCANCODE_LEFT,   BTN_LEFT  },
    { SDL_SCANCODE_RIGHT,  BTN_RIGHT },
    { SDL_SCANCODE_UP,     BTN_UP    },
    { SDL_SCANCODE_DOWN,   BTN_DOWN  },
    { SDL_SCANCODE_SPACE,  BTN_A     },
    { SDL_SCANCODE_Z,      BTN_A     },
    { SDL_SCANCODE_C,      BTN_B     },
    { SDL_SCANCODE_LSHIFT, BTN_B     },
    { SDL_SCANCODE_RETURN, BTN_START },
    { SDL_SCANCODE_P,      BTN_START }
};

static uint32_t held_buttons(void)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint32_t b = 0;
    size_t i;

    for (i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
        if (k[keymap[i].key]) {
            b |= keymap[i].button;
        }
    }
    return b;
}

static uint32_t button_of(SDL_Scancode key)
{
    size_t i;

    for (i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
        if (keymap[i].key == key) {
            return keymap[i].button;
        }
    }
    return 0;
}

static uint32_t load_hiscore(void)
{
    FILE *f = fopen(HISCORE_FILE, "rb");
    uint32_t v = 0;

    if (f) {
        if (fread(&v, sizeof(v), 1, f) != 1) {
            v = 0;
        }
        fclose(f);
    }
    return v;
}

static void save_hiscore(uint32_t v)
{
    FILE *f = fopen(HISCORE_FILE, "wb");

    if (f) {
        fwrite(&v, sizeof(v), 1, f);
        fclose(f);
    }
}

int main(int argc, char **argv)
{
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    gfx_t g;
    bot_t bot;
    int scale = 2;
    int demo = 0;
    int running = 1;
    uint32_t latched = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            scale = atoi(argv[++i]);
            if (scale < 1) scale = 1;
            if (scale > 6) scale = 6;
        } else if (!strcmp(argv[i], "--demo")) {
            demo = 1;
        } else if (!strcmp(argv[i], "--help")) {
            printf("usage: %s [--scale N] [--demo]\n\n"
                   "  --scale N  window magnification, 1..6 (default 2)\n"
                   "  --demo     let the built-in bot play\n\n"
                   "keys: arrows move/rotate/soft drop, Space or Z = A (hard drop),\n"
                   "      C or Left Shift = B (hold), Enter or P = START (pause),\n"
                   "      Escape quits\n", argv[0]);
            return 0;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    win = SDL_CreateWindow(demo ? "TETRIS 240x320 - demo"
                                : "TETRIS 240x320 - arrows, Space=A, C=B, Enter=START",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           TETRIS_SCREEN_W * scale, TETRIS_SCREEN_H * scale,
                           SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(ren, TETRIS_SCREEN_W, TETRIS_SCREEN_H);

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
                            SDL_TEXTUREACCESS_STREAMING,
                            TETRIS_SCREEN_W, TETRIS_SCREEN_H);
    if (!tex) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    g.buf = framebuffer;
    g.w = TETRIS_SCREEN_W;
    g.h = TETRIS_SCREEN_H;
    g.x_off = 0;
    g.y_off = 0;

    tetris_init(SDL_GetTicks() ^ 0xA5A5F00Du, load_hiscore());
    bot_reset(&bot);

    while (running) {
        SDL_Event ev;
        uint32_t start = SDL_GetTicks();
        uint32_t buttons;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
                /* Remember presses that arrive and end between two polls,
                   otherwise a very short tap would be lost entirely. */
                latched |= button_of(ev.key.keysym.scancode);
            }
            /* Mouse and touch events are intentionally not handled. */
        }

        buttons = demo ? bot_step(&bot) : (held_buttons() | latched);
        latched = 0;

        tetris_tick(start, buttons);
        tetris_render(&g);

        if (tetris_take_highscore_dirty()) {
            save_hiscore(tetris_highscore());
        }

        SDL_UpdateTexture(tex, NULL, framebuffer, TETRIS_SCREEN_W * (int)sizeof(uint16_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        {
            uint32_t spent = SDL_GetTicks() - start;
            if (spent < TETRIS_FRAME_MS) {
                SDL_Delay(TETRIS_FRAME_MS - spent);
            }
        }
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
