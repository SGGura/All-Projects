/*
 * shots.c - headless harness: plays the game with a scripted bot, checks
 * invariants and writes PNG screenshots. Needs no display and no SDL, so it
 * runs in CI.
 *
 *   usage: tetris_shots [output_dir]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "tetris.h"
#include "png_write.h"

#define W TETRIS_SCREEN_W
#define H TETRIS_SCREEN_H

static uint16_t fb[W * H];
static uint16_t fb_bands[W * H];

static gfx_t full = { fb, W, H, 0, 0 };

static uint32_t clock_ms;
static char out_dir[256] = "shots";
static int failures;

static void check(int ok, const char *what)
{
    if (!ok) {
        printf("  FAIL  %s\n", what);
        failures++;
    }
}

static void step(uint32_t buttons)
{
    tetris_tick(clock_ms, buttons);
    clock_ms += TETRIS_FRAME_MS;
}

/* A button press only registers on its rising edge, so tap = press, release. */
static void tap(uint32_t button)
{
    step(button);
    step(0);
}

static void idle(int frames)
{
    while (frames-- > 0) {
        step(0);
    }
}

static void shot(const char *name)
{
    char path[512];

    tetris_render(&full);
    snprintf(path, sizeof(path), "%s/%s.png", out_dir, name);
    if (png_write_rgb565(path, fb, W, H) != 0) {
        printf("  FAIL  cannot write %s\n", path);
        failures++;
    } else {
        printf("  wrote %s\n", path);
    }
}

/* ------------------------------------------------------------------ */
/* Bot                                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t cell[TETRIS_FIELD_H][TETRIS_FIELD_W];
} grid_t;

static void grab_grid(grid_t *g)
{
    int x, y;

    for (y = 0; y < TETRIS_FIELD_H; y++) {
        for (x = 0; x < TETRIS_FIELD_W; x++) {
            g->cell[y][x] = tetris_cell(x, y) ? 1 : 0;
        }
    }
}

static int hits(const grid_t *g, uint16_t s, int px, int py)
{
    int x, y;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            int bx = px + x;
            int by = py + y;

            if (!(s & (1u << (y * 4 + x)))) {
                continue;
            }
            if (bx < 0 || bx >= TETRIS_FIELD_W || by >= TETRIS_FIELD_H) {
                return 1;
            }
            if (by >= 0 && g->cell[by][bx]) {
                return 1;
            }
        }
    }
    return 0;
}

/* Classic four-feature evaluation: prefer clearing lines, punish height,
   covered holes and a jagged surface. */
static int evaluate(const grid_t *g)
{
    int heights[TETRIS_FIELD_W];
    int x, y;
    int total_height = 0, holes = 0, bumpiness = 0, lines = 0;

    for (x = 0; x < TETRIS_FIELD_W; x++) {
        int top = TETRIS_FIELD_H;

        for (y = 0; y < TETRIS_FIELD_H; y++) {
            if (g->cell[y][x]) {
                top = y;
                break;
            }
        }
        heights[x] = TETRIS_FIELD_H - top;
        total_height += heights[x];

        for (y = top + 1; y < TETRIS_FIELD_H; y++) {
            if (!g->cell[y][x]) {
                holes++;
            }
        }
    }
    for (x = 0; x + 1 < TETRIS_FIELD_W; x++) {
        int d = heights[x] - heights[x + 1];
        bumpiness += (d < 0) ? -d : d;
    }
    for (y = 0; y < TETRIS_FIELD_H; y++) {
        int full_row = 1;

        for (x = 0; x < TETRIS_FIELD_W; x++) {
            if (!g->cell[y][x]) {
                full_row = 0;
                break;
            }
        }
        lines += full_row;
    }

    return -51 * total_height + 760 * lines - 360 * holes - 18 * bumpiness;
}

/* Picks a rotation and column for the falling piece, then plays it. */
static void bot_place_piece(void)
{
    grid_t base;
    int piece, rot, col, row;
    int best_score = -(1 << 30);
    int best_rot = 0, best_col = 0;
    int r, c;
    int rotations, i;

    if (!tetris_active(&piece, &rot, &col, &row)) {
        idle(2);
        return;
    }
    grab_grid(&base);

    for (r = 0; r < 4; r++) {
        uint16_t s = tetris_shape_bits(piece, r);

        for (c = -3; c < TETRIS_FIELD_W; c++) {
            grid_t g = base;
            int py = -4;
            int x, y, score;

            if (hits(&g, s, c, py)) {
                continue;
            }
            while (!hits(&g, s, c, py + 1)) {
                py++;
            }
            if (py < -1) {
                continue; /* would lock outside the well */
            }
            for (y = 0; y < 4; y++) {
                for (x = 0; x < 4; x++) {
                    if ((s & (1u << (y * 4 + x))) && py + y >= 0) {
                        g.cell[py + y][c + x] = 1;
                    }
                }
            }
            score = evaluate(&g);
            if (score > best_score) {
                best_score = score;
                best_rot = r;
                best_col = c;
            }
        }
    }

    rotations = (best_rot - rot) & 3;
    for (i = 0; i < rotations; i++) {
        tap(BTN_UP);
    }

    if (!tetris_active(&piece, &rot, &col, &row)) {
        return;
    }
    while (col != best_col) {
        int before = col;

        tap(col < best_col ? BTN_RIGHT : BTN_LEFT);
        if (!tetris_active(&piece, &rot, &col, &row)) {
            return;
        }
        if (col == before) {
            break; /* blocked by a wall or the stack */
        }
    }

    tap(BTN_A);
}

/* ------------------------------------------------------------------ */
/* Checks                                                              */
/* ------------------------------------------------------------------ */

static void check_no_settled_full_rows(void)
{
    int x, y;

    if (tetris_state() != TETRIS_RUNNING) {
        return;
    }
    for (y = 0; y < TETRIS_FIELD_H; y++) {
        int full_row = 1;

        for (x = 0; x < TETRIS_FIELD_W; x++) {
            if (!tetris_cell(x, y)) {
                full_row = 0;
                break;
            }
        }
        if (full_row) {
            check(0, "a completed row survived into the running state");
            return;
        }
    }
}

static void check_band_rendering(void)
{
    /* A microcontroller without room for a 150 KB frame renders in bands. The
       result has to be identical to the full-frame path. */
    enum { BAND = 40 };
    uint16_t band[W * BAND];
    gfx_t g = { band, W, BAND, 0, 0 };
    int y;

    tetris_render(&full);

    for (y = 0; y < H; y += BAND) {
        g.y_off = (int16_t)y;
        tetris_render(&g);
        memcpy(&fb_bands[(size_t)y * W], band, sizeof(band));
    }
    check(memcmp(fb, fb_bands, sizeof(fb)) == 0,
          "banded rendering matches full-frame rendering");
}

int main(int argc, char **argv)
{
    uint32_t prev_score = 0, prev_lines = 0;
    int pieces = 0;
    int got_clear_shot = 0;
    int got_mid_shot = 0;
    int i;

    if (argc > 1) {
        snprintf(out_dir, sizeof(out_dir), "%s", argv[1]);
    }

    printf("tetris headless harness -> %s\n", out_dir);

    tetris_init(0xC0FFEEu, 12340);
    idle(1);

    check(tetris_state() == TETRIS_TITLE, "starts on the title screen");
    shot("01_title");
    check_band_rendering();

    tap(BTN_A);
    idle(1);
    check(tetris_state() == TETRIS_RUNNING, "A starts the game");
    check(tetris_active(NULL, NULL, NULL, NULL), "a piece is falling");

    for (i = 0; i < 6; i++) {
        bot_place_piece();
        idle(4);
        pieces++;
    }
    shot("02_early_game");

    /* Play until the bot dies or the stack has had a long life. */
    while (pieces < 900 && tetris_state() != TETRIS_GAMEOVER) {
        if (tetris_state() == TETRIS_RUNNING) {
            bot_place_piece();
            pieces++;
        } else {
            idle(1);
        }

        check(tetris_score() >= prev_score, "score never decreases");
        check(tetris_lines() >= prev_lines, "line count never decreases");
        check(tetris_level() >= 1 && tetris_level() <= 20, "level stays in range");
        prev_score = tetris_score();
        prev_lines = tetris_lines();
        check_no_settled_full_rows();

        if (!got_clear_shot && tetris_state() == TETRIS_CLEARING) {
            shot("03_line_clear");
            got_clear_shot = 1;
        }
        if (!got_mid_shot && pieces >= 120 && tetris_state() == TETRIS_RUNNING) {
            shot("04_mid_game");
            check_band_rendering();
            got_mid_shot = 1;

            tap(BTN_START);
            check(tetris_state() == TETRIS_PAUSED, "START pauses");
            shot("05_paused");
            tap(BTN_START);
            check(tetris_state() == TETRIS_RUNNING, "START resumes");
        }
        if (failures > 20) {
            break;
        }
    }

    check(got_clear_shot, "the bot completed at least one line");
    printf("  played %d pieces, score %u, lines %u, level %d\n",
           pieces, tetris_score(), tetris_lines(), tetris_level());

    /* Stack up quickly with unrotated drops to reach the game over screen. */
    while (tetris_state() != TETRIS_GAMEOVER && pieces < 1400) {
        if (tetris_state() == TETRIS_RUNNING) {
            tap(BTN_A);
            pieces++;
        } else {
            idle(1);
        }
    }
    check(tetris_state() == TETRIS_GAMEOVER, "topping out ends the game");
    check(tetris_highscore() >= tetris_score(), "high score tracks the score");
    shot("06_game_over");

    tap(BTN_A);
    check(tetris_state() == TETRIS_TITLE, "A returns to the title screen");

    /* Hold swaps the falling piece and only works once per piece. */
    tap(BTN_A);
    idle(1);
    {
        int piece_before = -1, piece_after = -1, piece_again = -1;

        tetris_active(&piece_before, NULL, NULL, NULL);
        check(tetris_hold_piece() < 0, "hold starts empty");
        tap(BTN_B);
        tetris_active(&piece_after, NULL, NULL, NULL);
        check(tetris_hold_piece() == piece_before, "hold stores the piece");
        check(piece_after != piece_before || piece_before < 0,
              "hold brings in a different piece");
        tap(BTN_B);
        tetris_active(&piece_again, NULL, NULL, NULL);
        check(piece_again == piece_after, "hold is limited to once per piece");
    }

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
