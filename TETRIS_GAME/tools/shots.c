/*
 * shots.c - headless harness: plays the game with the bot, checks invariants
 * and writes PNG screenshots. Needs no display and no SDL, so it runs in CI.
 *
 *   usage: tetris_shots [output_dir]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "tetris.h"
#include "bot.h"
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

/* A button only registers on its rising edge, so tap = press, release. */
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
    bot_t bot;
    uint32_t prev_score = 0, prev_lines = 0;
    int frames = 0;
    int drops = 0;
    int got_clear_shot = 0;
    int got_early_shot = 0;
    int got_mid_shot = 0;

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

    bot_reset(&bot);

    while (frames < 60000 && tetris_state() != TETRIS_GAMEOVER) {
        uint32_t buttons = bot_step(&bot);

        step(buttons);
        frames++;
        if (buttons & BTN_A) {
            drops++;
        }

        check(tetris_score() >= prev_score, "score never decreases");
        check(tetris_lines() >= prev_lines, "line count never decreases");
        check(tetris_level() >= 1 && tetris_level() <= 20, "level stays in range");
        prev_score = tetris_score();
        prev_lines = tetris_lines();
        check_no_settled_full_rows();

        if (!got_early_shot && drops >= 6 && tetris_state() == TETRIS_RUNNING) {
            shot("02_early_game");
            got_early_shot = 1;
        }
        if (!got_clear_shot && tetris_state() == TETRIS_CLEARING) {
            shot("03_line_clear");
            got_clear_shot = 1;
        }
        if (!got_mid_shot && drops >= 120 && tetris_state() == TETRIS_RUNNING) {
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
    check(got_mid_shot, "the bot survived at least 120 pieces");
    printf("  dropped %d pieces over %d frames, score %u, lines %u, level %d\n",
           drops, frames, tetris_score(), tetris_lines(), tetris_level());

    /* Stack up with unrotated drops to reach the game over screen. */
    while (tetris_state() != TETRIS_GAMEOVER && frames < 90000) {
        if (tetris_state() == TETRIS_RUNNING) {
            tap(BTN_A);
            frames += 2;
        } else {
            idle(1);
            frames++;
        }
    }
    check(tetris_state() == TETRIS_GAMEOVER, "topping out ends the game");
    check(tetris_highscore() >= tetris_score(), "high score tracks the score");
    shot("06_game_over");

    /* The bot may still be holding A from the drop that topped it out, and a
       button only acts on its rising edge, so let go first. */
    idle(2);
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

    /* A single frame long press still registers, which is what a port that
       polls slowly, or a very short tap on a real button, produces. */
    {
        int col_before = -1, col_after = -1;

        tetris_active(NULL, NULL, &col_before, NULL);
        step(BTN_LEFT);
        step(0);
        tetris_active(NULL, NULL, &col_after, NULL);
        check(col_after == col_before - 1, "a one frame tap moves exactly one column");
    }

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
