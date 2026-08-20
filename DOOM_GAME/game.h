/*
 * DOOM-240 — Wolfenstein-style raycaster for 240x320 TFT, button controls.
 *
 * Target: PIC32MZ2048EFH (MIPS M-Class @ 200-252 MHz, hardware FPU, 512 KB RAM)
 *         + LVGL 8.x. Also runs on the desktop SDL simulator (see main.c).
 *
 * The game core is pure C99 with no LVGL dependency. It renders every frame
 * into a caller-provided RGB565 framebuffer of GAME_W x GAME_H pixels
 * (row-major, index = y * GAME_W + x). On the MCU wrap that buffer in an
 * lv_canvas (LV_IMG_CF_TRUE_COLOR, LV_COLOR_DEPTH 16) and invalidate it after
 * each game_tick(), or blit it to the display directly for maximum FPS.
 *
 * Requirements on the LVGL side:
 *   - LV_COLOR_DEPTH   16
 *   - LV_COLOR_16_SWAP 0  (if your display driver needs swapped bytes,
 *                          compile the game with -DGAME_COLOR_SWAP=1 instead:
 *                          the finished frame is byte-swapped in one pass)
 *
 * Integration sketch (PIC32 side):
 *
 *   static lv_color_t game_fb[GAME_W * GAME_H];        // 150 KB in RAM
 *   lv_obj_t *cv = lv_canvas_create(lv_scr_act());
 *   lv_canvas_set_buffer(cv, game_fb, GAME_W, GAME_H, LV_IMG_CF_TRUE_COLOR);
 *   game_init((uint16_t *)game_fb);
 *   for (;;) {
 *       game_set_key(GAME_KEY_UP,   btn_up_pressed());   // poll your buttons
 *       game_set_key(GAME_KEY_DOWN, btn_down_pressed());
 *       game_set_key(GAME_KEY_LEFT, btn_left_pressed());
 *       game_set_key(GAME_KEY_RIGHT,btn_right_pressed());
 *       game_set_key(GAME_KEY_FIRE, btn_fire_pressed());
 *       game_tick(ms_since_last_call);
 *       lv_obj_invalidate(cv);
 *       lv_timer_handler();
 *   }
 *
 * Controls (5 buttons): UP/DOWN move, LEFT/RIGHT turn, FIRE shoots,
 * starts the game and restarts after death/victory.
 */

#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#define GAME_W 240
#define GAME_H 320

typedef enum {
    GAME_KEY_UP = 0,
    GAME_KEY_DOWN,
    GAME_KEY_LEFT,
    GAME_KEY_RIGHT,
    GAME_KEY_FIRE,
    GAME_KEY_COUNT
} game_key_t;

/* framebuffer: GAME_W*GAME_H uint16_t (RGB565), owned by the caller */
void game_init(uint16_t *framebuffer);

/* report current (held) state of a button; call whenever the state changes
 * or simply every frame before game_tick() */
void game_set_key(game_key_t key, bool pressed);

/* advance the game by dt_ms milliseconds and render the frame */
void game_tick(uint32_t dt_ms);

#endif /* GAME_H */
