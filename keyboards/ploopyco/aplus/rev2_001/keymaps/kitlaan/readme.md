# ploopyco/aplus:kitlaan

The A+ is not in upstream `qmk_firmware` yet. It lives on the `ploopyco/a+`
branch of [ploopyco/qmk_firmware](https://github.com/ploopyco/qmk_firmware).
This keymap therefore builds against a sibling clone, not the pinned submodule,
and it is absent from `qmk.json` because CI cannot build it.

## Build

`keyboards/ploopyco/aplus/.envrc` points `QMK_FIRMWARE_ROOT` at the sibling
clone, so a build needs no extra flags:

```bash
cd keyboards/ploopyco/aplus/rev2_001/keymaps/kitlaan
qmk compile
```

Every build prints an off-pin warning. That is correct here, because the
firmware is not the pinned SHA.

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

## Files

| File | Holds |
| --- | --- |
| `keymap.c` | the layers, the QMK hooks, and `process_record_user` |
| `wheels.c` | the two TMAG5273 wheel sensors and all scroll processing |
| `gestures.c` | the mouse gesture actions |
| `kitlaan.h` | `user_config_t`, and the colour, keycode and layer enums |

`pointing_device_gestures[]` has to stay in `keymap.c`: the gestures module
reads it from its own `introspection.c` with no extern declaration, so it only
resolves where the keymap is compiled.

## Differences from the Ploopy default keymap

* No left-hand mode.
* The keymap sits under `rev2_001`, not the shared `aplus/keymaps`, so that
  `qmk compile` can infer `-kb` and `-km`. Ploopy keeps keymaps at the parent
  level, which has no `keyboard.json` and thus defeats inference.

## When upstream carries the A+

1. Bump the `qmk_firmware` submodule to a SHA with `keyboards/ploopyco/aplus`.
2. Delete `keyboards/ploopyco/aplus/.envrc`.
3. `qmk userspace-add -kb ploopyco/aplus/rev2_001 -km kitlaan`, and remove the
   note in `qmk.json`.
4. Remove the `modules/drashna` submodule once upstream supplies
   `drashna/pointing_device_gestures`. The userspace copy shadows the
   firmware's, so a stale copy here wins silently.

Expect the keyboard name to move. Review can rename `aplus`, restructure the
revision directory, or add a `ploopyco/aplus` alias that points at the current
revision, as the other Ploopy boards have.
