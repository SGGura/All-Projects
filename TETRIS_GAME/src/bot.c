#include "bot.h"
#include "tetris.h"

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

/* Classic four feature evaluation: reward completed lines, punish height,
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

static void plan(bot_t *b, int piece)
{
    grid_t base;
    int best = -(1 << 30);
    int r, c;

    grab_grid(&base);
    b->target_rot = 0;
    b->target_col = 3;

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
                continue; /* would come to rest outside the well */
            }
            for (y = 0; y < 4; y++) {
                for (x = 0; x < 4; x++) {
                    if ((s & (1u << (y * 4 + x))) && py + y >= 0) {
                        g.cell[py + y][c + x] = 1;
                    }
                }
            }
            score = evaluate(&g);
            if (score > best) {
                best = score;
                b->target_rot = r;
                b->target_col = c;
            }
        }
    }
}

void bot_reset(bot_t *b)
{
    b->have_plan = 0;
    b->release = 0;
    b->stuck = 0;
    b->last_col = 0;
}

uint32_t bot_step(bot_t *b)
{
    int piece, rot, col, row;

    if (!tetris_active(&piece, &rot, &col, &row)) {
        /* Between pieces, and on the title / game over screens. */
        b->have_plan = 0;
        b->release = 0;
        return (tetris_state() == TETRIS_TITLE || tetris_state() == TETRIS_GAMEOVER)
               ? BTN_A : 0;
    }

    if (b->release) {
        b->release = 0;
        return 0;
    }

    if (!b->have_plan) {
        plan(b, piece);
        b->have_plan = 1;
        b->stuck = 0;
        b->last_col = -100; /* sentinel: no move attempted yet */
    }

    b->release = 1;

    if (rot != b->target_rot) {
        return BTN_UP;
    }

    if (col != b->target_col) {
        if (col == b->last_col) {
            if (++b->stuck > 2) {
                b->have_plan = 0; /* wall or stack in the way, drop as is */
                return BTN_A;
            }
        } else {
            b->stuck = 0;
        }
        b->last_col = col;
        return (col < b->target_col) ? BTN_RIGHT : BTN_LEFT;
    }

    b->have_plan = 0;
    return BTN_A;
}
