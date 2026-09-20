#!/usr/bin/env bash

set -eEuo pipefail

userspacePath="$1"

# The image already ships the QMK CLI, and firmware comes from the submodule.
# Do not pip install qmk, and do not clone a second copy.

# The devcontainer runs as a different user than the one that owns the checkout,
# so git needs these marked safe.
# The dockerized shim runs as the host user and so does not need to.
git config --global --add safe.directory "$userspacePath"
git config --global --add safe.directory "$userspacePath/qmk_firmware"

# This also pulls qmk_firmware's own submodules; first setup will be slow.
git submodule update --init --recursive

# A devcontainer persists, so config survives here.
qmk config user.qmk_home="$userspacePath/qmk_firmware"
qmk config user.overlay_dir="$userspacePath"

# The same check gates the CI build.
qmk userspace-doctor
