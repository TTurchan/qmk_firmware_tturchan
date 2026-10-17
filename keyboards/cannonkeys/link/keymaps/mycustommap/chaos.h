// chaos.h: keystroke-fed Markov chain driving a cellular automaton, one per
// keyboard half, with the two halves coupled over the split link.
//
// Pure C with no QMK dependencies, so the same file builds into firmware and
// into the host simulator in sim/.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef CHAOS_W
#    define CHAOS_W 128
#endif
#ifndef CHAOS_H
#    define CHAOS_H 32
#endif
#define CHAOS_PAGES (CHAOS_H / 8)
#define CHAOS_BYTES (CHAOS_W * CHAOS_PAGES)

#define CHAOS_STATES 16
#define CHAOS_KEYQ 8
#define CHAOS_ZONES 16 // 4 x 4 grid of screen regions the listener watches

// The grid is stored in SSD1306 page order (byte = 8 vertical pixels,
// index = page * W + x) so it can go straight to oled_write_raw().
typedef struct {
    uint8_t  grid[2][CHAOS_BYTES];
    uint8_t  cur;
    uint8_t  ghost_w[CHAOS_PAGES]; // partner's east edge, our x = -1
    uint8_t  ghost_e[CHAOS_PAGES]; // partner's west edge, our x = W
    uint8_t  trans[CHAOS_STATES][CHAOS_STATES]; // learned key-to-key bigrams
    uint8_t  state;                             // Markov chain position
    uint8_t  last_sym;                          // last key symbol we heard
    uint8_t  beat;                              // frames until next Markov step
    uint8_t  heat;                              // typing intensity, decays
    uint16_t x;                                 // logistic map, Q16
    uint16_t partner_x;
    uint8_t  partner_state;
    uint32_t rng;
    uint32_t pool; // entropy pool folded from the keystroke bitstream
    uint32_t tick;
    uint16_t pop;
    bool     is_left;
    // Feedback loop. The listener's chain learns from what the speaker's
    // automaton does instead of from keystrokes; the speaker's chain in turn
    // learns from the listener's walk (its "echo").
    bool     listener;
    uint8_t  zone_pop[CHAOS_ZONES]; // last observed population per zone
    uint8_t  obs, obs_seq;          // speaker: latest observation symbol
    uint8_t  echo, echo_seq;        // listener: latest Markov step
    uint8_t  heard_seq, heard_last; // what we already trained on from the partner
} chaos_t;

// What one half tells the other, both directions. Edges are this half's own
// leftmost and rightmost columns. 27 bytes at 128x32, 35 at 128x64, 51 at
// 64x128.
typedef struct __attribute__((packed)) {
    uint8_t  state;
    uint8_t  heat;
    uint16_t x;
    uint32_t pool;
    uint8_t  edge_w[CHAOS_PAGES];
    uint8_t  edge_e[CHAOS_PAGES];
    uint8_t  sym, sym_seq; // speaker: CA observation; listener: echo
    uint8_t  nkeys;
    uint8_t  keys[CHAOS_KEYQ]; // row<<4 | col, bit 7 unused
} chaos_packet_t;

// listener: this half's chain learns from the other half's automaton, not
// from keys. Make exactly one half the listener, or neither for the
// symmetric behaviour.
void chaos_init(chaos_t *c, uint32_t seed, bool is_left, bool listener);

// A key went down on this half. row/col are local to the half (0-based).
void chaos_key(chaos_t *c, uint8_t row, uint8_t col, uint16_t time);

// Advance one frame.
void chaos_step(chaos_t *c);

void chaos_export(const chaos_t *c, chaos_packet_t *p);
void chaos_absorb(chaos_t *c, const chaos_packet_t *p);

static inline const uint8_t *chaos_frame(const chaos_t *c) {
    return c->grid[c->cur];
}
