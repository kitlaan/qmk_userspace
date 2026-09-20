# QMK Userspace

My keymaps, built against a pinned `qmk_firmware`. Forked from
[qmk/qmk_userspace](https://github.com/qmk/qmk_userspace).

## Quick start

```bash
qmk userspace-compile                      # build everything in qmk.json
qmk compile -kb planck/rev6 -km kitlaan    # build one target
qmk flash -kb planck/rev6 -km kitlaan      # build and flash
```

The commands are the same whether the CLI is installed natively, running in the
devcontainer, or running in docker. Firmware is always the pinned submodule,
unless `qmk config` already has a `user.qmk_home` from an earlier `qmk setup`
on this host. Run `qmk config user.qmk_home` to check.

## Setup

```bash
git clone --recurse-submodules <this repo>
cd qmk_userspace
direnv allow    # see .envrc
```

Then pick one:

- **Native**: install the QMK CLI, per the
  [QMK docs](https://docs.qmk.fm/#/newbs). `.envrc` detects it.
- **Devcontainer**: run `git submodule update --init --recursive` on the host
  first (otherwise it'll leave root-owned files).
- **Docker**: `.envrc` puts `util/bin/qmk` on `PATH`, which runs the CLI in a
  container. Every call runs `--privileged` with `/dev` mounted, because
  flashing needs a device node that appears only after a board reset. Set
  `SKIP_FLASHING_SUPPORT=1` to drop both and give up flashing.

If neither `qmk` nor `docker` is available, nothing works.

`.envrc` exports `QMK_USERSPACE` and `QMK_HOME`, and adds the shim only when no
native `qmk` exists. Put machine-specific settings in `.envrc.local`.

Do not run `qmk config user.qmk_home=...` through the shim. It'll touch
`.qmk-home/` and overrides qmk_firmware pinning. Delete `.qmk-home/` to fix it.

## Where keymaps live

| Path | Use |
|---|---|
| `keyboards/<kb>/keymaps/kitlaan/` | one specific board |
| `layouts/<layout>/kitlaan/` | any board with that physical layout |
| `users/kitlaan/` | code shared by all of them |
| `modules/` | community modules |

Reminder: a new *keyboard* lives in `qmk_firmware/keyboards`, so point the
submodule at those changes with `git submodule set-url`.

### Cookbook

```bash
qmk list-keyboards | grep -i planck            # find the exact keyboard name

qmk userspace-add -kb planck/rev6 -km kitlaan  # creates the keymap if missing,
                                               # then registers the build target

qmk userspace-list                             # what will be built

qmk userspace-remove -kb planck/rev6 -km kitlaan

qmk userspace-compile                          # everything in qmk.json; this is
                                               # what CI builds

cd keyboards/planck/rev6/keymaps/kitlaan && qmk compile   # keyboard and keymap
                                               # both inferred from the directory

qmk userspace-doctor                           # CI gates on this

qmk git-submodule                              # update firmware's own submodules
```

## Firmware pin and override

`qmk_firmware` is a submodule, so local builds and GitHub releases use one SHA.
To update it:

```bash
git submodule update --remote qmk_firmware
qmk git-submodule                              # update firmware's own submodules too
git commit -am "Bump qmk_firmware"
```

To build against a different checkout for one command:

```bash
QMK_FIRMWARE_ROOT=~/Projects/qmk_firmware qmk compile -kb planck/rev6 -km kitlaan
```

There's a warning if the checkout has drifted off the pinned SHA.

For work *inside* a separate `qmk_firmware` clone, copy
`util/firmware.envrc.example` to that clone's `.envrc`. Firmware clone needs
to be a sibling of this repository.

## GitHub Actions

Push, and `.github/workflows/build_binaries.yaml` builds every target in
`qmk.json` and publishes the firmware under Releases.

The workflow should build with the pinned qmk_firmware submodule.
