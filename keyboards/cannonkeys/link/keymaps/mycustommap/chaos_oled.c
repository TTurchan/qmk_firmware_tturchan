// chaos_oled.c: runs one chaos_t per half and couples them over the split
// link with a user RPC transaction.
//
// Only the master sees keystrokes (process_record_user never runs on the
// slave), so the master routes each press to the half it physically came
// from: its own keys feed its own model, the other half's keys ride along in
// the next sync packet.
#include "chaos_oled.h"

#include <string.h>
#include "transactions.h"
#include "chaos.h"

#ifndef CHAOS_FRAME_MS
#    define CHAOS_FRAME_MS 66
#endif
// Which half listens to the other's automaton: 0 = neither (symmetric),
// 1 = left listens, 2 = right listens.
#ifndef CHAOS_LISTENER
#    define CHAOS_LISTENER 2
#endif
#ifndef CHAOS_SYNC_MS
#    define CHAOS_SYNC_MS 50
#endif

_Static_assert(sizeof(chaos_packet_t) <= RPC_M2S_BUFFER_SIZE, "raise RPC_M2S_BUFFER_SIZE");
_Static_assert(sizeof(chaos_packet_t) <= RPC_S2M_BUFFER_SIZE, "raise RPC_S2M_BUFFER_SIZE");
_Static_assert(CHAOS_W == OLED_DISPLAY_WIDTH && CHAOS_H == OLED_DISPLAY_HEIGHT, "CHAOS_W/H must match the OLED");

static chaos_t        chaos;
static chaos_packet_t outbox; // master: keys queued for the other half
static chaos_packet_t inbox;  // slave: last packet from the master
static chaos_packet_t answer; // slave: what we reply with
static volatile bool  inbox_full;

// Runs on the slave's split transport thread. The main loop may be mid-way
// through updating `answer`; a torn packet is just more chaos, so no lock.
static void chaos_slave_sync(uint8_t in_len, const void *in, uint8_t out_len, void *out) {
    if (in_len == sizeof(inbox)) {
        memcpy(&inbox, in, sizeof(inbox));
        inbox_full = true;
    }
    if (out_len == sizeof(answer)) memcpy(out, &answer, sizeof(answer));
}

static uint32_t hardware_seed(void) {
    uint32_t s = timer_read32() * 0x9E3779B9u;
#if defined(MCU_RP)
    // RP2040 ring oscillator RANDOMBIT (ROSC base 0x40060000 + 0x1c): real,
    // if lumpy, entropy. Good enough to make every boot different.
    volatile uint32_t *randombit = (volatile uint32_t *)0x4006001Cu;
    for (int i = 0; i < 32; i++) s = (s << 1 | (s >> 31)) ^ (*randombit & 1);
#endif
    return s ^ (is_keyboard_left() ? 0x13579BDFu : 0x2468ACE0u);
}

void chaos_oled_init(void) {
    bool left = is_keyboard_left();
    chaos_init(&chaos, hardware_seed(), left, CHAOS_LISTENER == (left ? 1 : 2));
    transaction_register_rpc(CHAOS_SYNC, chaos_slave_sync);
}

void chaos_oled_record(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    if (!record->event.pressed || record->event.type != KEY_EVENT) return; // skip combos, encoders
    uint8_t row = record->event.key.row, col = record->event.key.col;
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return;

    // Split matrices put the left half in the first MATRIX_ROWS/2 rows,
    // whichever half is master.
    bool    key_left = row < MATRIX_ROWS / 2;
    uint8_t lrow     = row % (MATRIX_ROWS / 2);
    if (key_left == is_keyboard_left()) {
        chaos_key(&chaos, lrow, col, record->event.time);
    } else if (outbox.nkeys < CHAOS_KEYQ) {
        outbox.keys[outbox.nkeys++] = (uint8_t)(lrow << 4 | (col & 15));
    }
}

void chaos_oled_task(void) {
    static uint32_t frame_timer, sync_timer;

    if (inbox_full) {
        inbox_full = false;
        chaos_absorb(&chaos, &inbox);
    }

    if (timer_elapsed32(frame_timer) >= CHAOS_FRAME_MS) {
        frame_timer = timer_read32();
        chaos_step(&chaos);
        if (!is_keyboard_master()) {
            chaos_export(&chaos, &answer);
            answer.nkeys = 0;
        }
    }

    if (is_keyboard_master() && timer_elapsed32(sync_timer) >= CHAOS_SYNC_MS) {
        sync_timer = timer_read32();
        chaos_packet_t reply;
        chaos_export(&chaos, &outbox); // leaves the key queue alone
        if (transaction_rpc_exec(CHAOS_SYNC, sizeof(outbox), &outbox, sizeof(reply), &reply)) {
            outbox.nkeys = 0;
            chaos_absorb(&chaos, &reply);
        }
    }
}

bool chaos_oled_render(void) {
    oled_set_cursor(0, 0);
    oled_write_raw((const char *)chaos_frame(&chaos), CHAOS_BYTES);
    return false;
}
