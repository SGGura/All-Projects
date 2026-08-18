#include "tetris.h"

/* ------------------------------------------------------------------ */
/* Playfield geometry                                                  */
/* ------------------------------------------------------------------ */

#define FIELD_W     10
#define FIELD_H     20
#define HIDDEN_ROWS 2                        /* spawn/kick space above the well */
#define ROWS        (FIELD_H + HIDDEN_ROWS)
#define CELL        14

#define FIELD_X     6
#define FIELD_Y     34
#define FIELD_PX_W  (FIELD_W * CELL)         /* 140 */
#define FIELD_PX_H  (FIELD_H * CELL)         /* 280 */
#define BORDER      2

#define PANEL_X     154
#define PANEL_W     80

/* ------------------------------------------------------------------ */
/* Feel / timing, all in milliseconds                                  */
/* ------------------------------------------------------------------ */

#define DAS_MS         170   /* delay before horizontal auto-repeat starts */
#define ARR_MS          45   /* auto-repeat interval                       */
#define SOFT_DROP_MS    45
#define LOCK_DELAY_MS  500
#define LOCK_MAX_RESET  15
#define CLEAR_ANIM_MS  260
#define BLINK_MS       450

/* ------------------------------------------------------------------ */
/* Palette                                                             */
/* ------------------------------------------------------------------ */

#define COL_BG        GFX_RGB(9, 12, 24)
#define COL_WELL      GFX_RGB(17, 21, 40)
#define COL_GRID      GFX_RGB(30, 36, 62)
#define COL_BORDER    GFX_RGB(64, 84, 136)
#define COL_BOX       GFX_RGB(14, 18, 34)
#define COL_TEXT      GFX_RGB(224, 232, 248)
#define COL_DIM       GFX_RGB(118, 132, 166)
#define COL_ACCENT    GFX_RGB(255, 198, 62)
#define COL_SHADE     GFX_RGB(6, 8, 16)

/* Tetromino base colours, index 0 is unused (empty cell). */
static const uint8_t piece_rgb[8][3] = {
    {  0,   0,   0},
    {  0, 198, 222}, /* I */
    {240, 202,  38}, /* O */
    {172,  82, 224}, /* T */
    { 58, 200,  86}, /* S */
    {232,  64,  72}, /* Z */
    { 52,  96, 224}, /* J */
    {242, 140,  30}  /* L */
};

static uint16_t col_base[8];
static uint16_t col_light[8];
static uint16_t col_shadow[8];
static uint16_t col_edge[8];
static uint16_t col_ghost[8];

/* ------------------------------------------------------------------ */
/* Tetromino shapes                                                    */
/* ------------------------------------------------------------------ */

/* Cells packed as bit (y * 4 + x) inside a 4x4 box, spawn orientation. */
static const uint16_t spawn_shape[7] = {
    0x00F0, /* I */
    0x0066, /* O */
    0x0072, /* T */
    0x0036, /* S */
    0x0063, /* Z */
    0x0071, /* J */
    0x0074  /* L */
};

#define PIECE_I 0
#define PIECE_O 1

static uint16_t shape[7][4];

/* Super Rotation System wall kicks, already converted to y-down screen
   coordinates. Index is from_rotation * 2, plus 1 for counter-clockwise. */
static const int8_t kick_jlstz[8][5][2] = {
    {{0, 0}, {-1,  0}, {-1, -1}, {0,  2}, {-1,  2}}, /* 0 -> R */
    {{0, 0}, { 1,  0}, { 1, -1}, {0,  2}, { 1,  2}}, /* 0 -> L */
    {{0, 0}, { 1,  0}, { 1,  1}, {0, -2}, { 1, -2}}, /* R -> 2 */
    {{0, 0}, { 1,  0}, { 1,  1}, {0, -2}, { 1, -2}}, /* R -> 0 */
    {{0, 0}, { 1,  0}, { 1, -1}, {0,  2}, { 1,  2}}, /* 2 -> L */
    {{0, 0}, {-1,  0}, {-1, -1}, {0,  2}, {-1,  2}}, /* 2 -> R */
    {{0, 0}, {-1,  0}, {-1,  1}, {0, -2}, {-1, -2}}, /* L -> 0 */
    {{0, 0}, {-1,  0}, {-1,  1}, {0, -2}, {-1, -2}}  /* L -> 2 */
};

static const int8_t kick_i[8][5][2] = {
    {{0, 0}, {-2,  0}, { 1,  0}, {-2,  1}, { 1, -2}}, /* 0 -> R */
    {{0, 0}, {-1,  0}, { 2,  0}, {-1, -2}, { 2,  1}}, /* 0 -> L */
    {{0, 0}, {-1,  0}, { 2,  0}, {-1, -2}, { 2,  1}}, /* R -> 2 */
    {{0, 0}, { 2,  0}, {-1,  0}, { 2, -1}, {-1,  2}}, /* R -> 0 */
    {{0, 0}, { 2,  0}, {-1,  0}, { 2, -1}, {-1,  2}}, /* 2 -> L */
    {{0, 0}, { 1,  0}, {-2,  0}, { 1,  2}, {-2, -1}}, /* 2 -> R */
    {{0, 0}, { 1,  0}, {-2,  0}, { 1,  2}, {-2, -1}}, /* L -> 0 */
    {{0, 0}, {-2,  0}, { 1,  0}, {-2,  1}, { 1, -2}}  /* L -> 2 */
};

/* Gravity per level, tuned for button play rather than for the guideline. */
static const uint16_t gravity_ms[20] = {
    800, 700, 600, 500, 420, 350, 290, 240, 190, 150,
    120, 100,  85,  70,  60,  55,  50,  45,  40,  35
};

static const uint16_t line_score[5] = {0, 100, 300, 500, 800};

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

#define NEXT_COUNT 3

static struct {
    uint8_t  board[ROWS][FIELD_W];

    int      state;
    uint32_t now;
    uint32_t seed;

    int      piece, rot, px, py;
    int      has_piece;

    uint8_t  bag[7];
    int      bag_pos;
    uint8_t  next_q[NEXT_COUNT];

    int      hold;         /* -1 when empty */
    int      hold_used;

    uint32_t score, high, lines;
    int      level;
    int      combo;
    int      back_to_back;
    int      high_dirty;
    int      new_record;

    uint32_t gravity_at;
    uint32_t lock_at;
    int      lock_resets;
    int      resting;

    uint32_t prev_buttons;
    int      move_dir;
    uint32_t move_at;

    uint32_t clear_at;
    uint32_t clear_rows;   /* one bit per board row, including hidden rows */
    int      clear_count;
} G;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static uint32_t rnd(void)
{
    /* xorshift32: no libc, deterministic, good enough to shuffle a bag */
    G.seed ^= G.seed << 13;
    G.seed ^= G.seed >> 17;
    G.seed ^= G.seed << 5;
    return G.seed;
}

static uint16_t rotate_cw(uint16_t s, int n)
{
    uint16_t out = 0;
    int x, y;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if (s & (1u << (y * 4 + x))) {
                out |= (uint16_t)(1u << (x * 4 + (n - 1 - y)));
            }
        }
    }
    return out;
}

static void build_shapes(void)
{
    int p, r;

    for (p = 0; p < 7; p++) {
        int box = (p == PIECE_I) ? 4 : 3;

        shape[p][0] = spawn_shape[p];
        for (r = 1; r < 4; r++) {
            shape[p][r] = (p == PIECE_O) ? spawn_shape[p]
                                         : rotate_cw(shape[p][r - 1], box);
        }
    }
}

static void build_palette(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        const uint8_t *c = piece_rgb[i];

        col_base[i]   = gfx_rgb_scaled(c[0], c[1], c[2], 100);
        col_light[i]  = gfx_rgb_scaled(c[0], c[1], c[2], 155);
        col_shadow[i] = gfx_rgb_scaled(c[0], c[1], c[2], 55);
        col_edge[i]   = gfx_rgb_scaled(c[0], c[1], c[2], 28);
        col_ghost[i]  = gfx_rgb_scaled(c[0], c[1], c[2], 45);
    }
}

static void fill_bag(void)
{
    int i;

    for (i = 0; i < 7; i++) {
        G.bag[i] = (uint8_t)i;
    }
    for (i = 6; i > 0; i--) {
        int j = (int)(rnd() % (uint32_t)(i + 1));
        uint8_t t = G.bag[i];
        G.bag[i] = G.bag[j];
        G.bag[j] = t;
    }
    G.bag_pos = 0;
}

static uint8_t bag_pull(void)
{
    if (G.bag_pos >= 7) {
        fill_bag();
    }
    return G.bag[G.bag_pos++];
}

static int collides(int piece, int rot, int px, int py)
{
    uint16_t s = shape[piece][rot];
    int x, y;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            int bx, by;

            if (!(s & (1u << (y * 4 + x)))) {
                continue;
            }
            bx = px + x;
            by = py + y;

            if (bx < 0 || bx >= FIELD_W || by >= ROWS) {
                return 1;
            }
            if (by >= 0 && G.board[by][bx]) {
                return 1;
            }
        }
    }
    return 0;
}

static int shape_min_y(uint16_t s)
{
    int y, x;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if (s & (1u << (y * 4 + x))) {
                return y;
            }
        }
    }
    return 0;
}

static void spawn(int piece)
{
    G.piece = piece;
    G.rot = 0;
    G.px = 3;
    G.py = HIDDEN_ROWS - shape_min_y(shape[piece][0]);
    G.has_piece = 1;
    G.lock_at = 0;
    G.lock_resets = 0;
    G.resting = 0;
    G.gravity_at = G.now;

    if (collides(G.piece, G.rot, G.px, G.py)) {
        G.has_piece = 0;
        G.state = TETRIS_GAMEOVER;
        if (G.score > G.high) {
            G.high = G.score;
            G.high_dirty = 1;
            G.new_record = 1;
        }
    }
}

static void spawn_from_queue(void)
{
    int piece = G.next_q[0];
    int i;

    for (i = 0; i < NEXT_COUNT - 1; i++) {
        G.next_q[i] = G.next_q[i + 1];
    }
    G.next_q[NEXT_COUNT - 1] = bag_pull();

    G.hold_used = 0;
    spawn(piece);
}

static int ghost_y(void)
{
    int y = G.py;

    while (!collides(G.piece, G.rot, G.px, y + 1)) {
        y++;
    }
    return y;
}

static void level_up_check(void)
{
    int lv = 1 + (int)(G.lines / 10u);

    G.level = (lv > 20) ? 20 : lv;
}

static void lock_piece(void)
{
    uint16_t s = shape[G.piece][G.rot];
    int x, y, row;
    int cleared = 0;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            int bx = G.px + x;
            int by = G.py + y;

            if ((s & (1u << (y * 4 + x))) && by >= 0 && by < ROWS) {
                G.board[by][bx] = (uint8_t)(G.piece + 1);
            }
        }
    }
    G.has_piece = 0;

    G.clear_rows = 0;
    for (row = 0; row < ROWS; row++) {
        int full = 1;

        for (x = 0; x < FIELD_W; x++) {
            if (!G.board[row][x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            G.clear_rows |= (uint32_t)1u << row;
            cleared++;
        }
    }
    G.clear_count = cleared;

    if (cleared > 0) {
        uint32_t gain = (uint32_t)line_score[cleared] * (uint32_t)G.level;

        if (cleared == 4) {
            if (G.back_to_back) {
                gain += 400u * (uint32_t)G.level;
            }
            G.back_to_back = 1;
        } else {
            G.back_to_back = 0;
        }

        G.combo++;
        if (G.combo > 0) {
            gain += 50u * (uint32_t)G.combo * (uint32_t)G.level;
        }

        G.score += gain;
        G.lines += (uint32_t)cleared;
        level_up_check();

        G.state = TETRIS_CLEARING;
        G.clear_at = G.now;
    } else {
        G.combo = -1;
        spawn_from_queue();
    }

    if (G.score > G.high) {
        G.high = G.score;
        G.high_dirty = 1;
        G.new_record = 1;
    }
}

static void collapse_rows(void)
{
    int src = ROWS - 1;
    int dst = ROWS - 1;

    while (src >= 0) {
        if (G.clear_rows & ((uint32_t)1u << src)) {
            src--;
            continue;
        }
        if (dst != src) {
            int x;
            for (x = 0; x < FIELD_W; x++) {
                G.board[dst][x] = G.board[src][x];
            }
        }
        src--;
        dst--;
    }
    while (dst >= 0) {
        int x;
        for (x = 0; x < FIELD_W; x++) {
            G.board[dst][x] = 0;
        }
        dst--;
    }
    G.clear_rows = 0;
    G.clear_count = 0;
}

static void note_lock_reset(void)
{
    if (G.resting && G.lock_resets < LOCK_MAX_RESET) {
        G.lock_resets++;
        G.lock_at = G.now;
    }
}

static int try_move(int dx, int dy)
{
    if (collides(G.piece, G.rot, G.px + dx, G.py + dy)) {
        return 0;
    }
    G.px += dx;
    G.py += dy;
    return 1;
}

static int try_rotate(int cw)
{
    int from = G.rot;
    int to = cw ? ((from + 1) & 3) : ((from + 3) & 3);
    int idx = from * 2 + (cw ? 0 : 1);
    const int8_t (*table)[2] = (G.piece == PIECE_I) ? kick_i[idx] : kick_jlstz[idx];
    int k;

    if (G.piece == PIECE_O) {
        return 0;
    }

    for (k = 0; k < 5; k++) {
        int nx = G.px + table[k][0];
        int ny = G.py + table[k][1];

        if (!collides(G.piece, to, nx, ny)) {
            G.rot = to;
            G.px = nx;
            G.py = ny;
            return 1;
        }
    }
    return 0;
}

static void do_hold(void)
{
    int cur = G.piece;

    if (G.hold_used) {
        return;
    }
    G.hold_used = 1;

    if (G.hold < 0) {
        G.hold = cur;
        spawn_from_queue();
    } else {
        int prev = G.hold;
        G.hold = cur;
        spawn(prev);
    }
    G.hold_used = 1;
}

static void start_game(void)
{
    int x, y, i;

    for (y = 0; y < ROWS; y++) {
        for (x = 0; x < FIELD_W; x++) {
            G.board[y][x] = 0;
        }
    }
    G.score = 0;
    G.lines = 0;
    G.level = 1;
    G.combo = -1;
    G.back_to_back = 0;
    G.hold = -1;
    G.hold_used = 0;
    G.new_record = 0;
    G.clear_rows = 0;
    G.clear_count = 0;
    G.move_dir = 0;

    G.bag_pos = 7;
    for (i = 0; i < NEXT_COUNT; i++) {
        G.next_q[i] = bag_pull();
    }

    G.state = TETRIS_RUNNING;
    spawn_from_queue();
}

/* ------------------------------------------------------------------ */
/* Public: lifecycle                                                   */
/* ------------------------------------------------------------------ */

void tetris_init(uint32_t seed, uint32_t highscore)
{
    build_shapes();
    build_palette();

    G.seed = seed ? seed : 0x1234567Du;
    G.high = highscore;
    G.high_dirty = 0;
    G.state = TETRIS_TITLE;
    G.has_piece = 0;
    G.now = 0;
    G.prev_buttons = 0;
    G.score = 0;
    G.lines = 0;
    G.level = 1;
    G.hold = -1;
}

int      tetris_state(void)     { return G.state; }
uint32_t tetris_score(void)     { return G.score; }
uint32_t tetris_highscore(void) { return G.high; }
uint32_t tetris_lines(void)     { return G.lines; }
int      tetris_level(void)     { return G.level; }

int tetris_take_highscore_dirty(void)
{
    int d = G.high_dirty;

    G.high_dirty = 0;
    return d;
}

uint16_t tetris_shape_bits(int piece, int rot)
{
    if (piece < 0 || piece > 6) {
        return 0;
    }
    return shape[piece][rot & 3];
}

int tetris_cell(int col, int row)
{
    if (col < 0 || col >= FIELD_W || row < 0 || row >= FIELD_H) {
        return 0;
    }
    return G.board[row + HIDDEN_ROWS][col];
}

int tetris_active(int *piece, int *rot, int *col, int *row)
{
    if (!G.has_piece) {
        return 0;
    }
    if (piece) *piece = G.piece;
    if (rot)   *rot   = G.rot;
    if (col)   *col   = G.px;
    if (row)   *row   = G.py - HIDDEN_ROWS;
    return 1;
}

int tetris_next(int index)
{
    if (index < 0 || index >= NEXT_COUNT) {
        return -1;
    }
    return G.next_q[index];
}

int tetris_hold_piece(void)
{
    return G.hold;
}

/* ------------------------------------------------------------------ */
/* Public: update                                                      */
/* ------------------------------------------------------------------ */

static void tick_running(uint32_t buttons, uint32_t pressed)
{
    int dir = 0;
    uint32_t interval;
    int soft;

    if (pressed & BTN_START) {
        G.state = TETRIS_PAUSED;
        return;
    }

    if ((buttons & (BTN_LEFT | BTN_RIGHT)) == BTN_LEFT) {
        dir = -1;
    } else if ((buttons & (BTN_LEFT | BTN_RIGHT)) == BTN_RIGHT) {
        dir = 1;
    }

    if (dir != G.move_dir) {
        G.move_dir = dir;
        if (dir && try_move(dir, 0)) {
            note_lock_reset();
        }
        G.move_at = G.now + DAS_MS;
    } else if (dir && (int32_t)(G.now - G.move_at) >= 0) {
        if (try_move(dir, 0)) {
            note_lock_reset();
        }
        G.move_at = G.now + ARR_MS;
    }

    if (pressed & BTN_UP) {
        if (try_rotate(1)) {
            note_lock_reset();
        }
    }
    if (pressed & BTN_B) {
        do_hold();
        return;
    }

    if (pressed & BTN_A) {
        int gy = ghost_y();

        G.score += 2u * (uint32_t)(gy - G.py);
        G.py = gy;
        lock_piece();
        return;
    }

    soft = (buttons & BTN_DOWN) ? 1 : 0;
    interval = gravity_ms[G.level - 1];
    if (soft && SOFT_DROP_MS < interval) {
        interval = SOFT_DROP_MS;
    }

    if ((uint32_t)(G.now - G.gravity_at) >= interval) {
        G.gravity_at = G.now;
        if (try_move(0, 1)) {
            if (soft) {
                G.score += 1u;
            }
            G.lock_at = 0;
            G.lock_resets = 0;
        }
    }

    G.resting = collides(G.piece, G.rot, G.px, G.py + 1);
    if (G.resting) {
        if (G.lock_at == 0) {
            G.lock_at = G.now;
        }
        if ((uint32_t)(G.now - G.lock_at) >= LOCK_DELAY_MS ||
            G.lock_resets >= LOCK_MAX_RESET) {
            lock_piece();
        }
    } else {
        G.lock_at = 0;
    }
}

void tetris_tick(uint32_t now_ms, uint32_t buttons)
{
    uint32_t pressed = buttons & ~G.prev_buttons;

    G.now = now_ms;

    /* Stir the generator with real input timing so each game differs. */
    if (pressed) {
        G.seed ^= now_ms * 2654435761u;
    }

    switch (G.state) {
    case TETRIS_TITLE:
        if (pressed & (BTN_A | BTN_START)) {
            start_game();
        }
        break;

    case TETRIS_RUNNING:
        if (G.has_piece) {
            tick_running(buttons, pressed);
        }
        break;

    case TETRIS_CLEARING:
        if ((uint32_t)(G.now - G.clear_at) >= CLEAR_ANIM_MS) {
            collapse_rows();
            G.state = TETRIS_RUNNING;
            spawn_from_queue();
        }
        break;

    case TETRIS_PAUSED:
        if (pressed & (BTN_START | BTN_A)) {
            G.state = TETRIS_RUNNING;
            G.gravity_at = G.now;
            G.lock_at = 0;
            G.move_dir = 0;
        }
        break;

    case TETRIS_GAMEOVER:
        if (pressed & (BTN_A | BTN_START)) {
            G.state = TETRIS_TITLE;
        }
        break;

    default:
        break;
    }

    G.prev_buttons = buttons;
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */

static void draw_block(gfx_t *g, int x, int y, int size, int idx)
{
    gfx_fill_rect(g, x, y, size, size, col_edge[idx]);
    gfx_fill_rect(g, x + 1, y + 1, size - 2, size - 2, col_base[idx]);
    gfx_hline(g, x + 1, y + 1, size - 2, col_light[idx]);
    gfx_vline(g, x + 1, y + 1, size - 2, col_light[idx]);
    gfx_hline(g, x + 1, y + size - 2, size - 2, col_shadow[idx]);
    gfx_vline(g, x + size - 2, y + 1, size - 2, col_shadow[idx]);
}

static void draw_cell(gfx_t *g, int col, int row, int idx)
{
    if (row < HIDDEN_ROWS) {
        return; /* above the visible well */
    }
    draw_block(g, FIELD_X + col * CELL, FIELD_Y + (row - HIDDEN_ROWS) * CELL,
               CELL, idx);
}

static void shape_bbox(uint16_t s, int *x0, int *y0, int *x1, int *y1)
{
    int x, y;

    *x0 = 3; *y0 = 3; *x1 = 0; *y1 = 0;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if (s & (1u << (y * 4 + x))) {
                if (x < *x0) *x0 = x;
                if (y < *y0) *y0 = y;
                if (x > *x1) *x1 = x;
                if (y > *y1) *y1 = y;
            }
        }
    }
}

static void draw_preview(gfx_t *g, int bx, int by, int bw, int bh,
                         int piece, int cell)
{
    uint16_t s;
    int x0, y0, x1, y1, ox, oy, x, y;

    if (piece < 0) {
        return;
    }
    s = shape[piece][0];
    shape_bbox(s, &x0, &y0, &x1, &y1);

    ox = bx + (bw - (x1 - x0 + 1) * cell) / 2 - x0 * cell;
    oy = by + (bh - (y1 - y0 + 1) * cell) / 2 - y0 * cell;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if (s & (1u << (y * 4 + x))) {
                draw_block(g, ox + x * cell, oy + y * cell, cell, piece + 1);
            }
        }
    }
}

static void draw_label_box(gfx_t *g, const char *label, int y, int h)
{
    gfx_text(g, PANEL_X, y, label, COL_DIM, 1);
    gfx_fill_rect(g, PANEL_X, y + 10, PANEL_W, h, COL_BOX);
    gfx_frame(g, PANEL_X, y + 10, PANEL_W, h, 1, COL_BORDER);
}

static void draw_value(gfx_t *g, const char *label, int y, uint32_t value, int pad)
{
    char buf[12];

    gfx_text(g, PANEL_X, y, label, COL_DIM, 1);
    gfx_utoa(buf, value, pad);
    gfx_text_right(g, PANEL_X + PANEL_W, y + 10, buf, COL_TEXT, 2);
}

static void draw_well(gfx_t *g)
{
    int row, col;
    int blink_on = ((G.now / 70u) & 1u) != 0u;

    gfx_frame(g, FIELD_X - BORDER, FIELD_Y - BORDER,
              FIELD_PX_W + 2 * BORDER, FIELD_PX_H + 2 * BORDER,
              BORDER, COL_BORDER);
    gfx_fill_rect(g, FIELD_X, FIELD_Y, FIELD_PX_W, FIELD_PX_H, COL_WELL);

    for (row = 0; row <= FIELD_H; row++) {
        for (col = 0; col <= FIELD_W; col++) {
            gfx_pixel(g, FIELD_X + col * CELL - 1, FIELD_Y + row * CELL - 1,
                      COL_GRID);
        }
    }

    for (row = HIDDEN_ROWS; row < ROWS; row++) {
        int clearing = (G.clear_rows & ((uint32_t)1u << row)) != 0;

        for (col = 0; col < FIELD_W; col++) {
            uint8_t v = G.board[row][col];

            if (!v) {
                continue;
            }
            if (clearing) {
                int sy = FIELD_Y + (row - HIDDEN_ROWS) * CELL;

                gfx_fill_rect(g, FIELD_X + col * CELL, sy, CELL, CELL,
                              blink_on ? COL_TEXT : COL_ACCENT);
            } else {
                draw_cell(g, col, row, v);
            }
        }
    }

    if (G.has_piece && (G.state == TETRIS_RUNNING || G.state == TETRIS_PAUSED)) {
        uint16_t s = shape[G.piece][G.rot];
        int gy = ghost_y();
        int x, y;

        for (y = 0; y < 4; y++) {
            for (x = 0; x < 4; x++) {
                int row2 = gy + y;

                if ((s & (1u << (y * 4 + x))) && row2 >= HIDDEN_ROWS) {
                    gfx_frame(g, FIELD_X + (G.px + x) * CELL,
                              FIELD_Y + (row2 - HIDDEN_ROWS) * CELL,
                              CELL, CELL, 1, col_ghost[G.piece + 1]);
                }
            }
        }
        for (y = 0; y < 4; y++) {
            for (x = 0; x < 4; x++) {
                if (s & (1u << (y * 4 + x))) {
                    draw_cell(g, G.px + x, G.py + y, G.piece + 1);
                }
            }
        }
    }
}

static void draw_header(gfx_t *g)
{
    char buf[12];

    gfx_text(g, 6, 8, "TETRIS", COL_ACCENT, 2);
    gfx_text_right(g, 234, 6, "BEST", COL_DIM, 1);
    gfx_utoa(buf, G.high, 6);
    gfx_text_right(g, 234, 16, buf, COL_TEXT, 1);
    gfx_hline(g, 0, 28, TETRIS_SCREEN_W, COL_BORDER);
}

static void draw_panel(gfx_t *g)
{
    int i;
    int progress;

    draw_label_box(g, "NEXT", 32, 96);
    for (i = 0; i < NEXT_COUNT; i++) {
        draw_preview(g, PANEL_X, 42 + i * 32, PANEL_W, 32, G.next_q[i], 6);
    }

    draw_label_box(g, "HOLD", 140, 40);
    if (G.hold >= 0) {
        draw_preview(g, PANEL_X, 150, PANEL_W, 40, G.hold, 6);
    }

    draw_value(g, "SCORE", 196, G.score, 0);
    draw_value(g, "LEVEL", 228, (uint32_t)G.level, 0);
    draw_value(g, "LINES", 260, G.lines, 0);

    progress = (int)(G.lines % 10u) * PANEL_W / 10;
    gfx_fill_rect(g, PANEL_X, 300, PANEL_W, 6, COL_BOX);
    gfx_frame(g, PANEL_X, 300, PANEL_W, 6, 1, COL_BORDER);
    if (progress > 2) {
        gfx_fill_rect(g, PANEL_X + 1, 301, progress - 2, 4, COL_ACCENT);
    }
}

/* Darkens the well with alternating scanlines so overlay text stays readable
   without needing alpha blending. */
static void shade_well(gfx_t *g)
{
    int y;

    for (y = FIELD_Y; y < FIELD_Y + FIELD_PX_H; y += 2) {
        gfx_hline(g, FIELD_X, y, FIELD_PX_W, COL_SHADE);
    }
}

static void draw_message(gfx_t *g, int y, int h, const char *l1,
                         const char *l2, uint16_t c1)
{
    int x = FIELD_X - BORDER;
    int w = FIELD_PX_W + 2 * BORDER;

    gfx_fill_rect(g, x, y, w, h, COL_BOX);
    gfx_frame(g, x, y, w, h, 2, COL_BORDER);
    gfx_text_center(g, x + w / 2, y + 12, l1, c1, 2);
    if (l2) {
        gfx_text_center(g, x + w / 2, y + h - 20, l2, COL_DIM, 1);
    }
}

static void draw_title(gfx_t *g)
{
    static const char *keys[][2] = {
        {"LEFT RIGHT", "MOVE"},
        {"UP",         "ROTATE"},
        {"DOWN",       "SOFT DROP"},
        {"A",          "HARD DROP"},
        {"B",          "HOLD"},
        {"START",      "PAUSE"}
    };
    int i;
    int y = 150;

    gfx_clear(g, COL_BG);

    gfx_text_center(g, TETRIS_SCREEN_W / 2, 40, "TETRIS", COL_ACCENT, 4);
    gfx_hline(g, 40, 76, 160, COL_BORDER);
    gfx_text_center(g, TETRIS_SCREEN_W / 2, 86, "240X320 BUTTONS ONLY", COL_DIM, 1);

    for (i = 0; i < 7; i++) {
        draw_block(g, 36 + i * 24, 108, 22, i + 1);
    }

    gfx_text(g, 30, 132, "CONTROLS", COL_TEXT, 1);
    gfx_hline(g, 30, 142, 180, COL_BORDER);
    for (i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++) {
        gfx_text(g, 30, y, keys[i][0], COL_TEXT, 1);
        gfx_text_right(g, 210, y, keys[i][1], COL_DIM, 1);
        y += 14;
    }

    if (G.high) {
        char buf[12];

        gfx_text_center(g, TETRIS_SCREEN_W / 2, 246, "BEST SCORE", COL_DIM, 1);
        gfx_utoa(buf, G.high, 0);
        gfx_text_center(g, TETRIS_SCREEN_W / 2, 258, buf, COL_TEXT, 2);
    }

    if (((G.now / BLINK_MS) & 1u) == 0u) {
        gfx_text_center(g, TETRIS_SCREEN_W / 2, 292, "PRESS A TO START",
                        COL_ACCENT, 1);
    }
}

void tetris_render(gfx_t *g)
{
    if (G.state == TETRIS_TITLE) {
        draw_title(g);
        return;
    }

    gfx_clear(g, COL_BG);
    draw_header(g);
    draw_well(g);
    draw_panel(g);

    if (G.state == TETRIS_PAUSED) {
        shade_well(g);
        draw_message(g, 140, 68, "PAUSED", "START TO RESUME", COL_ACCENT);
    } else if (G.state == TETRIS_GAMEOVER) {
        char buf[12];

        shade_well(g);
        draw_message(g, 120, 100, "GAME", "PRESS A", COL_ACCENT);
        gfx_text_center(g, FIELD_X + FIELD_PX_W / 2, 152, "OVER", COL_ACCENT, 2);
        gfx_utoa(buf, G.score, 0);
        gfx_text_center(g, FIELD_X + FIELD_PX_W / 2, 176, buf, COL_TEXT, 2);
        if (G.new_record && ((G.now / BLINK_MS) & 1u) == 0u) {
            gfx_text_center(g, FIELD_X + FIELD_PX_W / 2, 194, "NEW RECORD",
                            COL_ACCENT, 1);
        }
    }
}
