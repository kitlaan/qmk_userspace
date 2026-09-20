# Resolves the userspace and firmware roots. Source this; do not execute it.
#
# Sets QMK_USERSPACE_ROOT, QMK_FW_ROOT and QMK_FW_MODE (submodule|external).

# Clear stale state from a prior source. A prior run may have set these and
# then hit the error branch below; without this, that stale value would
# survive the non-zero return.
unset QMK_FW_ROOT QMK_FW_MODE

QMK_USERSPACE_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)" || return 1

if [[ -n ${QMK_FIRMWARE_ROOT-} ]]; then
    # Setting the variable is the opt-in. The submodule stays in place.
    # Nothing needs to be deleted to override it.
    QMK_FW_ROOT="$(cd -- "$QMK_FIRMWARE_ROOT" && pwd)" || return 1
    QMK_FW_MODE=external

    # CI always builds the pinned SHA. Say plainly when this build will not
    # match the release.
    _pinned=$(git -C "$QMK_USERSPACE_ROOT" rev-parse HEAD:qmk_firmware 2>/dev/null) || _pinned=
    _head=$(git -C "$QMK_FW_ROOT" rev-parse HEAD 2>/dev/null) || _head=
    if [[ -n $_pinned && -n $_head && "$_pinned" != "$_head" ]]; then
        printf 'warning: off-pin firmware\n  pinned: %.8s\n  in use: %s (%.8s)\n' \
            "$_pinned" "$QMK_FW_ROOT" "$_head" >&2
    fi
    unset _pinned _head
elif [[ -f $QMK_USERSPACE_ROOT/qmk_firmware/requirements.txt ]]; then
    QMK_FW_ROOT="$QMK_USERSPACE_ROOT/qmk_firmware"
    QMK_FW_MODE=submodule
else
    echo "error: no qmk_firmware found. run: git submodule update --init" >&2
    return 1
fi
