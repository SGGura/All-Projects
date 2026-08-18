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

#define HISCORE_FILE "tetris_hiscore.dat"

static uint16_t framebuffer[TETRIS_SCREEN_W * TETRIS_SCREEN_H];

static uint32_t read_buttons(void)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint32_t b = 0;

    if (k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A]) b |= BTN_LEFT;
    if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) b |= BTN_RIGHT;
    if (k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W]) b |= BTN_UP;
    if (k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S]) b |= BTN_DOWN;
    if (k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_Z]) b |= BTN_A;
    if (k[SDL_SCANCODE_C]     || k[SDL_SCANCODE_LSHIFT]) b |= BTN_B;
    if (k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_P]) b |= BTN_START;

    return b;
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
    int scale = 2;
    int running = 1;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            scale = atoi(argv[++i]);
            if (scale < 1) scale = 1;
            if (scale > 6) scale = 6;
        } else if (!strcmp(argv[i], "--help")) {
            printf("usage: %s [--scale N]\n", argv[0]);
            return 0;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    win = SDL_CreateWindow("TETRIS 240x320",
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

    while (running) {
        SDL_Event ev;
        uint32_t start = SDL_GetTicks();

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN && !ev.key.repeat &&
                       ev.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
            /* Mouse and touch events are intentionally not handled. */
        }

        tetris_tick(start, read_buttons());
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
