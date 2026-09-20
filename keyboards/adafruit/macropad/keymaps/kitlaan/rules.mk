TAP_DANCE_ENABLE = yes
DEFERRED_EXEC_ENABLE = yes

SRC += encoder.c
SRC += oled.c
SRC += keymap_utils.c

# tap_dance_actions lives here, not in keymap.c, and keymap_introspection.c
# needs to see it. This compiles tapdance.c, so it must not also be in SRC.
INTROSPECTION_KEYMAP_C = tapdance.c
