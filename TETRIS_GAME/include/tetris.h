/*
 * tetris.h - button-driven falling block game for a 240x320 portrait display.
 *
 * The core is plain C99: no libc, no allocation, no floating point and no
 * knowledge of the panel or the buttons. A port has to do three things:
 *
 *   1. call tetris_init() once,
 *   2. call tetris_tick() with a millisecond timestamp and a button bitmask,
 *   3. call tetris_render() into an RGB565 buffer and push that to the panel.
 *
 * The button mask is a level (not edge) signal: set the bit for as long as the
 * button is held. Auto-repeat, edge detection and debounce timing are handled
 * here so the port only has to read and debounce the pins.
 */
#ifndef TETRIS_H
#define TETRIS_H

#include <stdint.h>
#include "gfx.h"

#define TETRIS_SCREEN_W 240
#define TETRIS_SCREEN_H 320

#define TETRIS_FIELD_W 10
#define TETRIS_FIELD_H 20

/* Recommended tick/render rate. Everything is timestamp driven, so a slower
   loop still plays correctly, it just looks less smooth. */
#define TETRIS_FRAME_MS 16

enum tetris_button {
    BTN_LEFT  = 1u << 0, /* move left                        */
    BTN_RIGHT = 1u << 1, /* move right                       */
    BTN_UP    = 1u << 2, /* rotate clockwise                 */
    BTN_DOWN  = 1u << 3, /* soft drop                        */
    BTN_A     = 1u << 4, /* hard drop, also confirm          */
    BTN_B     = 1u << 5, /* hold piece (optional button)     */
    BTN_START = 1u << 6  /* pause / resume (optional button) */
};

enum tetris_state {
    TETRIS_TITLE,
    TETRIS_RUNNING,
    TETRIS_CLEARING,
    TETRIS_PAUSED,
    TETRIS_GAMEOVER
};

/* seed must differ between runs, otherwise every game deals the same pieces.
   A free running timer sampled at the first button press works well. */
void tetris_init(uint32_t seed, uint32_t highscore);

void tetris_tick(uint32_t now_ms, uint32_t buttons);
void tetris_render(gfx_t *g);

int      tetris_state(void);
uint32_t tetris_score(void);
uint32_t tetris_highscore(void);
uint32_t tetris_lines(void);
int      tetris_level(void);

/* Set when the high score changed, cleared by this call. Ports can use it to
   decide when to write EEPROM/flash instead of writing every frame. */
int tetris_take_highscore_dirty(void);

/* Introspection, used by the test harness and by anything that wants to watch
   the game from outside (attract-mode bot, external HUD, LED strip, ...). */

/* Cells of a tetromino packed as bit (y * 4 + x) inside a 4x4 box. */
uint16_t tetris_shape_bits(int piece, int rot);
/* Visible well only: 0 when empty, 1..7 for a tetromino colour index. */
int tetris_cell(int col, int row);
/* Falling piece in visible-well coordinates. Returns 0 when there is none;
   row may be negative while the piece is still partly above the well. */
int tetris_active(int *piece, int *rot, int *col, int *row);
int tetris_next(int index);
int tetris_hold_piece(void);

#endif /* TETRIS_H */
