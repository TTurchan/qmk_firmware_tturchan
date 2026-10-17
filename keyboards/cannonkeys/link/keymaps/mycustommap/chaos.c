// chaos.c: see chaos.h.
//
// Pipeline, once per frame:
//   keystrokes -> entropy pool + Markov bigram weights + XOR stamps on grid
//   Markov chain walks on its own (one step per "beat"), its state picks the
//     automaton rule and drops a stamp where that state's key lives
//   coupled logistic map (r rises with typing heat) sets mutation pressure
//   automaton steps, using the partner half's edge columns as ghost cells
#include "chaos.h"

#include <string.h>

// Matrix shape of one half, used only to place key stamps on screen.
#ifndef CHAOS_HALF_ROWS
#    define CHAOS_HALF_ROWS 5
#endif
#ifndef CHAOS_HALF_COLS
#    define CHAOS_HALF_COLS 6
#endif
// Lily58 numbers columns from the outer pinky (0) inward; the CannonKeys
// Link numbers them from the inner column (0) outward.
#ifndef CHAOS_COL0_INNER
#    define CHAOS_COL0_INNER 0
#endif

typedef struct {
    uint16_t born;    // life-like: bit n set = birth on n neighbours
    uint16_t survive; // life-like: bit n set = survive on n neighbours
    uint8_t  wolfram; // non-zero = 1D elementary rule, scrolling mode
} rule_t;

#define B(n) (1u << (n))
static const rule_t rules[CHAOS_STATES] = {
    {B(3), B(2) | B(3), 0},                                                  // Life
    {B(3) | B(6), B(2) | B(3), 0},                                           // HighLife
    {B(3) | B(6) | B(7) | B(8), B(3) | B(4) | B(6) | B(7) | B(8), 0},        // Day & Night
    {B(2), 0, 0},                                                            // Seeds
    {B(3), B(1) | B(2) | B(3) | B(4) | B(5), 0},                             // Maze
    {B(3), B(4) | B(5) | B(6) | B(7) | B(8), 0},                             // Coral
    {B(4) | B(6) | B(7) | B(8), B(3) | B(5) | B(6) | B(7) | B(8), 0},        // Anneal
    {B(3) | B(5) | B(6) | B(7) | B(8), B(5) | B(6) | B(7) | B(8), 0},        // Diamoeba
    {B(3) | B(6), B(1) | B(2) | B(5), 0},                                    // 2x2
    {B(3) | B(6) | B(8), B(2) | B(4) | B(5), 0},                             // Morley
    {B(1) | B(3) | B(5) | B(7), B(1) | B(3) | B(5) | B(7), 0},               // Replicator
    {B(3) | B(5) | B(7), B(1) | B(3) | B(5) | B(8), 0},                      // Amoeba
    {0, 0, 30},                                                              // rule 30
    {0, 0, 110},                                                             // rule 110
    {0, 0, 90},                                                              // rule 90
    {0, 0, 150},                                                             // rule 150
};
#undef B

static uint32_t rnd(chaos_t *c) {
    uint32_t x = c->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return c->rng = x;
}

static inline bool px(const chaos_t *c, const uint8_t *g, int x, int y) {
    if (y < 0) y += CHAOS_H;
    if (y >= CHAOS_H) y -= CHAOS_H;
    if (x < 0) return (c->ghost_w[y >> 3] >> (y & 7)) & 1;
    if (x >= CHAOS_W) return (c->ghost_e[y >> 3] >> (y & 7)) & 1;
    return (g[(y >> 3) * CHAOS_W + x] >> (y & 7)) & 1;
}

static inline void flip(uint8_t *g, int x, int y) {
    x = ((x % CHAOS_W) + CHAOS_W) % CHAOS_W;
    y = ((y % CHAOS_H) + CHAOS_H) % CHAOS_H;
    g[(y >> 3) * CHAOS_W + x] ^= (uint8_t)(1u << (y & 7));
}

// Where a key (or a Markov state replaying one) lives on screen: the outer
// pinky column at the screen's outer edge, so the right half is mirrored.
static void locus(const chaos_t *c, uint8_t row, uint8_t col, int *x, int *y) {
    if (CHAOS_COL0_INNER) col = CHAOS_HALF_COLS - 1 - col;
    int lx = col * CHAOS_W / CHAOS_HALF_COLS + CHAOS_W / (2 * CHAOS_HALF_COLS);
    *x     = c->is_left ? lx : CHAOS_W - 1 - lx;
    *y     = row * CHAOS_H / CHAOS_HALF_ROWS + CHAOS_H / (2 * CHAOS_HALF_ROWS);
}

// XOR a 4x4 blob taken from 16 bits of `bits` centred on (x, y).
static void stamp(chaos_t *c, int x, int y, uint16_t bits) {
    uint8_t *g = c->grid[c->cur];
    for (int i = 0; i < 16; i++) {
        if (bits & (1u << i)) flip(g, x - 2 + (i & 3), y - 2 + (i >> 2));
    }
}

static uint8_t sym_of(uint8_t row, uint8_t col) {
    return (uint8_t)((row * CHAOS_HALF_COLS + col) % CHAOS_STATES);
}

void chaos_init(chaos_t *c, uint32_t seed, bool is_left, bool listener) {
    memset(c, 0, sizeof(*c));
    c->is_left   = is_left;
    c->listener  = listener;
    c->rng       = seed ? seed : 0x9E3779B9u;
    c->pool      = seed ^ 0xA5A5A5A5u;
    c->x         = 0x5555;
    c->partner_x = 0xAAAA;
    for (int i = 0; i < CHAOS_STATES; i++)
        for (int j = 0; j < CHAOS_STATES; j++) c->trans[i][j] = 1 + (rnd(c) & 3);
    for (int i = 0; i < CHAOS_BYTES; i++) c->grid[0][i] = (uint8_t)(rnd(c) & rnd(c)); // ~25% density
    c->state = rnd(c) % CHAOS_STATES;
    c->beat  = 8;
}

// Reinforce the transition *last -> s; halve the row if it saturates so
// recent habits outweigh old ones.
static void learn(chaos_t *c, uint8_t *last, uint8_t s, uint8_t amount) {
    uint8_t *row = c->trans[*last];
    if (row[s] > 255 - amount) {
        for (int j = 0; j < CHAOS_STATES; j++) row[j] = (row[j] >> 1) | 1;
    }
    row[s] += amount;
    *last = s;
}

void chaos_key(chaos_t *c, uint8_t row, uint8_t col, uint16_t time) {
    // The bitstream: position bits and the low timing bits of every press.
    uint32_t bits = ((uint32_t)row << 4 | col) ^ ((uint32_t)time << 8);
    c->pool       = ((c->pool << 7) | (c->pool >> 25)) ^ bits ^ (c->pool * 0x2C1B3C6Du);
    c->rng ^= c->pool;
    if (!c->rng) c->rng = 1;

    // The listener's chain is deaf to keys; it only learns from the speaker.
    if (!c->listener) learn(c, &c->last_sym, sym_of(row, col), 24);

    c->heat = c->heat > 255 - 24 ? 255 : c->heat + 24;

    int x, y;
    locus(c, row, col, &x, &y);
    stamp(c, x, y, (uint16_t)(c->pool ^ (c->pool >> 16)) | 0x0660); // keep a solid core
}

static void markov_step(chaos_t *c) {
    uint16_t sum = 0;
    for (int j = 0; j < CHAOS_STATES; j++) sum += c->trans[c->state][j];
    uint16_t r = rnd(c) % sum;
    uint8_t  n = 0;
    while (r >= c->trans[c->state][n]) r -= c->trans[c->state][n++];
    c->state = n;
    c->echo  = n;
    c->echo_seq++;

    // Dream: replay a key that maps to this state at its spot on the screen.
    uint8_t k = n + CHAOS_STATES * (rnd(c) & 1);
    if (k >= CHAOS_HALF_ROWS * CHAOS_HALF_COLS) k = n;
    int x, y;
    locus(c, k / CHAOS_HALF_COLS, k % CHAOS_HALF_COLS, &x, &y);
    stamp(c, x, y, (uint16_t)rnd(c));

    // Slow beats when idle, fast when typing; the partner's logistic value
    // jitters the length so the halves never settle into lockstep.
    int beat = 6 + ((c->pool >> (n & 15)) & 15) + (c->partner_x >> 13) - (c->heat >> 5);
    c->beat  = beat < 2 ? 2 : (uint8_t)beat;
}

// Coupled logistic maps: r runs from 3.70 (already chaotic) idle up to 3.99
// at full typing heat. Q16 state, Q12 r.
static uint16_t logistic(uint16_t x, uint16_t r) {
    uint32_t t = ((uint32_t)x * (65536u - x)) >> 16;
    uint32_t f = (t * r) >> 12;
    return f > 65535 ? 65535 : (uint16_t)f;
}

static void step_life(chaos_t *c, const rule_t *rl) {
    const uint8_t *g = c->grid[c->cur];
    uint8_t       *o = c->grid[c->cur ^ 1];
    uint16_t       pop = 0;
    memset(o, 0, CHAOS_BYTES);
    for (int y = 0; y < CHAOS_H; y++) {
        // Sliding 3-wide window of column sums.
        int a = px(c, g, -1, y - 1) + px(c, g, -1, y) + px(c, g, -1, y + 1);
        int b = px(c, g, 0, y - 1) + px(c, g, 0, y) + px(c, g, 0, y + 1);
        for (int x = 0; x < CHAOS_W; x++) {
            int  cc    = px(c, g, x + 1, y - 1) + px(c, g, x + 1, y) + px(c, g, x + 1, y + 1);
            bool alive = px(c, g, x, y);
            int  n     = a + b + cc - alive;
            if (alive ? (rl->survive >> n) & 1 : (rl->born >> n) & 1) {
                o[(y >> 3) * CHAOS_W + x] |= (uint8_t)(1u << (y & 7));
                pop++;
            }
            a = b;
            b = cc;
        }
    }
    c->cur ^= 1;
    c->pop = pop;
}

// 1D elementary automaton running down the screen's height. Each new
// generation appears at the inner edge and history scrolls outward.
static void step_wolfram(chaos_t *c, uint8_t rule) {
    uint8_t *g    = c->grid[c->cur];
    bool     left = c->is_left;
    int      src  = left ? CHAOS_W - 1 : 0; // newest column sits at the inner edge
    for (int p = 0; p < CHAOS_PAGES; p++) {
        uint8_t *row = g + p * CHAOS_W;
        if (left) memmove(row, row + 1, CHAOS_W - 1);
        else memmove(row + 1, row, CHAOS_W - 1);
    }
    int     prev = left ? CHAOS_W - 2 : 1;
    uint8_t col[CHAOS_PAGES] = {0};
    for (int y = 0; y < CHAOS_H; y++) {
        int idx = px(c, g, prev, y - 1) << 2 | px(c, g, prev, y) << 1 | px(c, g, prev, y + 1);
        if ((rule >> idx) & 1) col[y >> 3] |= (uint8_t)(1u << (y & 7));
    }
    uint16_t pop = 0;
    for (int p = 0; p < CHAOS_PAGES; p++) g[p * CHAOS_W + src] = col[p];
    for (int i = 0; i < CHAOS_BYTES; i++) pop += __builtin_popcount(g[i]);
    c->pop = pop;
}

// Split the screen into 4 x 4 zones and report the zone whose population
// moved the most since last frame: where the automaton is "talking".
static void observe(chaos_t *c) {
    const uint8_t *g      = c->grid[c->cur];
    const int      zw     = CHAOS_W / 4, zp = CHAOS_PAGES / 4;
    uint8_t        best   = 0;
    int            best_d = -1;
    for (int z = 0; z < CHAOS_ZONES; z++) {
        int x0 = (z & 3) * zw, p0 = (z >> 2) * zp;
        int pop = 0;
        for (int p = p0; p < p0 + zp; p++)
            for (int x = x0; x < x0 + zw; x++) pop += __builtin_popcount(g[p * CHAOS_W + x]);
        pop >>= 2; // a zone is at most 512 cells at 128x64 or 64x128: fits a byte
        int d = pop - c->zone_pop[z];
        if (d < 0) d = -d;
        if (d > best_d) best_d = d, best = (uint8_t)z;
        c->zone_pop[z] = (uint8_t)pop;
    }
    if (best_d > 0) {
        c->obs = best;
        c->obs_seq++;
    }
}

void chaos_step(chaos_t *c) {
    c->tick++;

    uint16_t r = 15155 + (uint16_t)(((uint32_t)c->heat * (16343 - 15155)) >> 8);
    uint32_t mix = (7u * logistic(c->x, r) + logistic(c->partner_x, r)) >> 3;
    c->x = mix < 64 ? 64 + (rnd(c) & 0xFF) : (uint16_t)mix;

    if (c->beat == 0 || --c->beat == 0) markov_step(c);

    // Density guard: an emptying world gets dreamed into, a saturated one
    // forces the chain onward to a different rule.
    if (c->pop < CHAOS_W * CHAOS_H / 48) {
        int x = rnd(c) % CHAOS_W, y = rnd(c) % CHAOS_H;
        stamp(c, x, y, (uint16_t)rnd(c));
    } else if (c->pop > CHAOS_W * CHAOS_H * 2 / 5) {
        c->beat = 1;
    }

    // Mutation pressure: typing heat plus the logistic map's high excursions.
    int flips = (c->heat >> 6) + (c->x > 0xF400 ? 3 : 0);
    for (int i = 0; i < flips; i++) flip(c->grid[c->cur], rnd(c) % CHAOS_W, rnd(c) % CHAOS_H);

    const rule_t *rl = &rules[c->state];
    if (rl->wolfram) step_wolfram(c, rl->wolfram);
    else step_life(c, rl);
    observe(c);

    if (c->heat) c->heat -= 1 + (c->heat >> 5);
    if ((c->tick & 255) == 0) {
        // Forgetting: weights drift back toward uniform when you stop typing.
        for (int i = 0; i < CHAOS_STATES; i++)
            for (int j = 0; j < CHAOS_STATES; j++) c->trans[i][j] -= c->trans[i][j] >> 3;
    }
}

void chaos_export(const chaos_t *c, chaos_packet_t *p) {
    const uint8_t *g = c->grid[c->cur];
    p->state = c->state;
    p->heat  = c->heat;
    p->x     = c->x;
    p->pool  = c->pool;
    // The listener sends back its walk, the speaker what its world is doing.
    p->sym     = c->listener ? c->echo : c->obs;
    p->sym_seq = c->listener ? c->echo_seq : c->obs_seq;
    for (int i = 0; i < CHAOS_PAGES; i++) {
        p->edge_w[i] = g[i * CHAOS_W];
        p->edge_e[i] = g[i * CHAOS_W + CHAOS_W - 1];
    }
}

void chaos_absorb(chaos_t *c, const chaos_packet_t *p) {
    // The two screens form one ring: left|right|left... so each half's ghost
    // columns are the partner's opposite edges.
    memcpy(c->ghost_w, p->edge_e, CHAOS_PAGES);
    memcpy(c->ghost_e, p->edge_w, CHAOS_PAGES);
    c->partner_x     = p->x;
    // Each half teaches the other the transition from its own state to the
    // partner's, so the chains slowly entangle.
    if (c->trans[c->state][p->state] < 250) c->trans[c->state][p->state] += 2;
    c->partner_state = p->state;
    c->pool ^= (p->pool << 13) | (p->pool >> 19);
    c->rng ^= p->pool;
    if (!c->rng) c->rng = 0xDEADBEEF;
    if (p->heat > c->heat) c->heat = (c->heat + p->heat) >> 1; // sympathetic warmth

    if (p->sym_seq != c->heard_seq) {
        c->heard_seq = p->sym_seq;
        uint8_t s    = p->sym % CHAOS_STATES;
        if (c->listener) {
            // Learn the speaker's automaton as if it were someone typing, and
            // echo the hot zone back as a mirror image across the seam.
            learn(c, &c->heard_last, s, 24);
            int zw = CHAOS_W / 4, zh = CHAOS_H / 4;
            int x  = (s & 3) * zw + zw / 2, y = (s >> 2) * zh + zh / 2;
            stamp(c, CHAOS_W - 1 - x, y, (uint16_t)(p->pool >> 8) | 0x0660);
            c->heat = c->heat > 255 - 2 ? 255 : c->heat + 2; // a simmer, not a boil
        } else {
            // The speaker hears the listener's walk and folds it into the
            // same table your typing trains. That closes the loop.
            learn(c, &c->heard_last, s, 12);
        }
    }

    uint8_t n = p->nkeys > CHAOS_KEYQ ? CHAOS_KEYQ : p->nkeys;
    for (uint8_t i = 0; i < n; i++) chaos_key(c, p->keys[i] >> 4, p->keys[i] & 15, (uint16_t)(p->pool >> (i * 3)));
}
