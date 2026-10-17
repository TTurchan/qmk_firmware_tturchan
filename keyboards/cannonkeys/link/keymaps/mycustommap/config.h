#pragma once

#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
#define IGNORE_MOD_TAP_INTERRUPT

// Chaos OLEDs: one user RPC transaction carries state both ways.
// Packet is 51 bytes at 64x128 — fits comfortably in 64-byte buffers.
#define SPLIT_TRANSACTION_IDS_USER CHAOS_SYNC
#define RPC_M2S_BUFFER_SIZE 64
#define RPC_S2M_BUFFER_SIZE 64

// Wake and sleep both screens together when typing activity changes.
#define SPLIT_ACTIVITY_ENABLE

// Five minutes without a keypress and both screens go dark.
#define OLED_TIMEOUT 300000

// The Link's SH1107 panels are 64 wide x 128 tall.
// OLED_DISPLAY_64X128 is set by the board's config.h.
#define CHAOS_W 64
#define CHAOS_H 128

// One half of the Link matrix: 5 rows x 6 cols, column 0 innermost.
#define CHAOS_HALF_ROWS 5
#define CHAOS_HALF_COLS 6
#define CHAOS_COL0_INNER 1

// 0 = both halves learn from keys, 1 = left listens to the right's automaton,
// 2 = right listens to the left's (and the left learns back from the right).
#define CHAOS_LISTENER 2

// ~15 fps simulation, 20 Hz sync between halves.
#define CHAOS_FRAME_MS 66
#define CHAOS_SYNC_MS  50
#define OLED_UPDATE_INTERVAL 33
