# macropad:kitlaan

A numpad by default, with per-game layers.

## Layers

| Layer | Purpose |
|---|---|
| `_NUMPAD` | Default. Numpad. |
| `_DIABLO3` | Game layer, with the skill spammers. |
| `_ELITEDANGEROUS` | Game layer. |
| `_MACRO` | The `M_ED_*` Elite Dangerous macros. |
| `_ADJUST` | Toggled from itself. Carries the encoder's layer switching. |

The encoder acts only on `_ADJUST`, where it cycles the default layer. It does
nothing on the other layers. No keycode reaches `_MACRO`, so that encoder cycle
is the only route to it.

On `_DIABLO3`, a skill key sends once on the first press. Tap it again in the
same dance to start repeating, where the tap count picks the period from
`diablo_times`: two taps is 1 s, three is 2 s, then 3, 5, 8, 13, 21 and 34 s.
A single tap stops a running repeat, and so does leaving the layer.

`_FUSION360` exists in `layers.h` but stays out of the build, because
`FEATURE_FUSION360` is commented out in `config.h`.

Uncommenting that define is not enough to use the layer. `keymap.c` has no
`[_FUSION360]` block, so the layer would be all `KC_NO` — and because the
encoder cycles the default layer, rotating onto it leaves no key that can get
back to `_ADJUST`. The pad then needs a replug. Add the `keymaps` entry and a
`LAYOUT_rot*` block at the same time as the define.

## Orientation

The pad is used rotated, so `keymap_utils.h` defines `LAYOUT_rotL` and
`LAYOUT_rotR`. Both wrap the stock `LAYOUT` and reorder the arguments, so a
layer can be written as it physically looks. `rotL` puts the encoder on the left
and the OLED on the right; `rotR` is the mirror. The game layers use `rotL`;
`_NUMPAD` and `_ADJUST` use the stock upright `LAYOUT`.

## Files

| File | Holds |
|---|---|
| `keymap.c` | Layers, custom keycodes, RGB per-layer colors |
| `keymap_utils.c` | The deferred-exec macro engine and layer helpers |
| `tapdance.c` | Tap-dance definitions |
| `encoder.c` | Encoder behaviour per layer |
| `oled.c` | OLED rendering |
| `layers.h` | Layer enum, gated by the `FEATURE_` defines |

`rules.mk` enables `TAP_DANCE_ENABLE` and `DEFERRED_EXEC_ENABLE`. The macro
engine needs the second one.

## Still to do

- Set the base colour pattern from the active layer: Diablo 3 red, Fusion 360
  undecided, numpad dimmed.
- On the adjust layer, colour by the layer it would switch to.

## Notes

`layers.h` declares `NUM_LAYERS` as the last enum member and then defines it
again as `(_ADJUST + 1)`. The two agree today. They stop agreeing if a layer is
added after `_ADJUST`, so add new layers before it.
