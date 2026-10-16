# ploopyco/aplus:kitlaan

The A+ is not in upstream `qmk_firmware` yet. It lives on the `ploopyco/a+`
branch of [ploopyco/qmk_firmware](https://github.com/ploopyco/qmk_firmware).
A local build uses a sibling clone. CI uses the pinned submodule, the same one
as `main`, and applies `ci/patches/ploopyco-aplus.patch` to it first.

## Build

`keyboards/ploopyco/aplus/.envrc` points `QMK_FIRMWARE_ROOT` at the sibling
clone, so a build needs no extra flags:

```bash
cd keyboards/ploopyco/aplus/rev2_001/keymaps/kitlaan
qmk compile
```

Every build prints an off-pin warning. That is correct here, because the
firmware is not the pinned SHA.

## Flash

1. Hold down the bottom-left button (left click, not the knob).
2. Connect the USB-C cable while you hold the button.
3. The board shows as a USB drive. Copy
   `ploopyco_aplus_rev2_001_kitlaan.uf2` onto it. The board then restarts.

Bootmagic also resets the EEPROM, so the DPI, brightness and scroll
toggles go back to their defaults. If bootmagic does not work, use the
hardware trigger in the board's own `readme.md`.

## The sibling clone

`../qmk_ploopy`, with `origin` at ploopyco/qmk_firmware and `upstream` at
qmk/qmk_firmware. Its `.envrc` is a copy of `util/firmware.envrc.example`.

Branch `kitlaan/aplus-on-pin` is derived, not authored. Keep `ploopyco/a+` as a
pristine mirror and put no work of your own in that repository, so a Ploopy
update — a force-push included — costs nothing. To rebuild the branch:

```bash
git fetch origin upstream
git checkout -B kitlaan/aplus-on-pin origin/ploopyco/a+
git rebase --onto $(git -C ../qmk_userspace rev-parse HEAD:qmk_firmware) \
    $(git merge-base origin/ploopyco/a+ origin/master)
git rm --cached modules/drashna && git commit -m "Drop the unregistered modules/drashna gitlink"
```

That last step is not optional. The A+ commit records a gitlink at
`modules/drashna` with no `.gitmodules` entry, and git then aborts every
recursive submodule command in the repository. The modules come from
`modules/drashna` in this userspace instead.

## CI

The CI patch is the net diff of that branch, less the parts this keymap does
not use, so regenerate it each time you rebuild the branch:

```bash
{
  echo "ploopyco/a+ $(git -C ../qmk_ploopy rev-parse origin/ploopyco/a+), rebased onto the pinned qmk_firmware. See the kitlaan keymap readme."
  git -C ../qmk_ploopy diff $(git rev-parse HEAD:qmk_firmware) kitlaan/aplus-on-pin --       keyboards/ploopyco/aplus keyboards/ploopyco/common ':!keyboards/ploopyco/aplus/keymaps'
} > ci/patches/ploopyco-aplus.patch
```

The patch leaves out two parts of the branch:

* Ploopy's `default` keymap, which CI does not build.
* Ploopy's changes to the shared `ploopyco.c` and `ploopyco.h`. They serve
  the `default` keymap only, and they define `is_drag_scroll` a second time
  in the header. This keymap uses its own drag scroll and keycodes, so the
  pinned versions work. A local build still uses Ploopy's versions.

It is a plain diff, not `format-patch`, because the branch adds and then drops
the `modules/drashna` gitlink, and only the net result applies cleanly.

Nothing watches Ploopy for updates. To check by hand, compare
`git -C ../qmk_ploopy ls-remote origin ploopyco/a+` with the SHA on the first
line of the patch.

A push builds the branch. It publishes the `latest-<branch>` release only while
the `RELEASE_BRANCHES` repo variable names the branch:

```bash
gh variable set RELEASE_BRANCHES --body '["ploopyco-aplus"]'
```

## Modes

A mode is a base layer plus the wheel behaviour that belongs with it. The
buttons come from the layer, but the wheels are read in
`pointing_device_task_user()` rather than from the keymap, so `modes[]` in
`keymap.c` carries a handler per mode. Only the base mode exists so far; the
framework is here for the ones to come.

Hold the right knob and roll the ball to pick a mode. Release without rolling
and it returns to the previous mode, which is the quick way back and forth.
A roll onto a direction that no mode claims blinks instead of switching.
`MODE_PICK_DIRECTIONS` in `config.h` chooses four cardinal rolls or all eight;
the cardinals keep their modes either way.

Either knob held also raises `LAYER_KNOB_HOLD`, so its six buttons mean the same
thing whichever knob you use. The knob only selects what the *ball* does: the
left one makes a mouse gesture, the right one picks a mode. Only the first knob
down gets the ball, because the gesture module tracks one session at a time.

Layers changed to suit this. `LAYER_CONTROL` moved to 14 and `LAYER_KNOB_HOLD`
is 15, because QMK resolves a key from the highest active layer and a mode is
the default layer. The layer count is 16 so that those two never move as modes
are added; see the comment on the layer enum.

## Files

| File | Holds |
| --- | --- |
| `keymap.c` | the layers, the `modes[]` table, and the QMK hooks |
| `modes.c` | the mode machinery and the knob-hold state behind it |
| `wheels.c` | the two TMAG5273 wheel sensors and all scroll processing |
| `gestures.c` | the mouse gesture actions |
| `kitlaan.h` | `user_config_t`, and the colour, keycode and layer enums |

`modes.c` keeps its own state private, so `keymap.c` reaches the knobs through
`mode_process_record()`, `mode_scan()` and `mode_task()` rather than sharing
flags with it.

`pointing_device_gestures[]` has to stay in `keymap.c`: the gestures module
reads it from its own `introspection.c` with no extern declaration, so it only
resolves where the keymap is compiled.

## Differences from the Ploopy default keymap

* No left-hand mode.
* The right knob taps to toggle the control layer and holds to pick a mode,
  where the default toggles control on press.
* `PKC_TGL_ACCEL` on the control layer. It does not persist: the accel module
  keeps that state in RAM only, so a replug turns acceleration back on.
* Drag scroll comes from `dragscroll.c`, not `is_drag_scroll` in
  `ploopyco.c`, so that it slows down on hosts without hi-res scroll (macOS).
  The divisors still come from the board's `config.h`.
* The keymap sits under `rev2_001`, not the shared `aplus/keymaps`, so that
  `qmk compile` can infer `-kb` and `-km`. Ploopy keeps keymaps at the parent
  level, which has no `keyboard.json` and thus defeats inference.

## When upstream carries the A+

1. Bump the `qmk_firmware` submodule to a SHA with `keyboards/ploopyco/aplus`.
2. Delete `keyboards/ploopyco/aplus/.envrc` and `ci/patches/ploopyco-aplus.patch`.
   The target is already in `qmk.json`.
3. Merge to `main`, and remove the branch from `RELEASE_BRANCHES`.
4. Remove the `modules/drashna` submodule once upstream supplies
   `drashna/pointing_device_gestures` and `drashna/pointing_device_accel`. The userspace copy shadows the
   firmware's, so a stale copy here wins silently.

Expect the keyboard name to move. Review can rename `aplus`, restructure the
revision directory, or add a `ploopyco/aplus` alias that points at the current
revision, as the other Ploopy boards have.
