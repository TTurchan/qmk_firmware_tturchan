COMBO_ENABLE = yes
LTO_ENABLE = yes

# Chaos OLEDs: chaos.c is the engine, chaos_oled.c the QMK glue.
OLED_ENABLE = yes
SRC += chaos.c chaos_oled.c
