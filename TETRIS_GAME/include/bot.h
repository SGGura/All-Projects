/*
 * bot.h - optional automatic player.
 *
 * Drives the game through the same button mask a human would produce, using
 * only the public introspection API. Used by the test harness and by the
 * simulator's attract mode; a minimal firmware port can simply leave bot.c
 * out of the build.
 */
#ifndef BOT_H
#define BOT_H

#include <stdint.h>

typedef struct {
    int target_rot;
    int target_col;
    int have_plan;
    int release;    /* a button must be let go before it can be pressed again */
    int last_col;
    int stuck;
} bot_t;

void bot_reset(bot_t *b);

/* Call once per frame and feed the result to tetris_tick(). */
uint32_t bot_step(bot_t *b);

#endif /* BOT_H */
