#!/usr/bin/env python3
"""Make a printable cheat sheet for the kitlaan A+ keymap.

The labels come from `@doc` tags in the C source, so the sheet follows the
keymap. See "Cheat sheet" in readme.md for the tags. The script stops with an
error when something on the board has no label, so that CI catches a sheet
that would otherwise drift from the firmware.

Standard library only, so that CI can run it with no setup.
"""

import argparse
import colorsys
import html
import math
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

KEYMAP_DIR = Path(__file__).resolve().parent
USERSPACE = KEYMAP_DIR.parents[5]
DEFAULT_OUTPUT = "ploopyco_aplus_rev2_001_kitlaan_cheatsheet.html"

# LAYOUT() argument order, from the board's info.json.
POSITIONS = ["tll", "tl", "tr", "trr", "bl", "br", "knob_left", "knob_right"]
POSITION_NAMES = {
    "tll": "Top left left",
    "tl": "Top left",
    "tr": "Top right",
    "trr": "Top right right",
    "bl": "Bottom left",
    "br": "Bottom right",
    "knob_left": "Knob left",
    "knob_right": "Knob right",
}

# Stock QMK keycodes. A keymap-specific meaning goes in a layer tag instead.
BUILTIN_LABELS = {
    "MS_BTN1": "Left click",
    "MS_BTN2": "Right click",
    "MS_BTN3": "Middle click",
    "MS_BTN4": "Back",
    "MS_BTN5": "Forward",
}
# The keycodes whose tap runs the mode's tap_left / tap_right.
KNOB_TAPS = {"PKC_GESTURE": "left", "PKC_MODE": "right"}
TRANSPARENT = {"_______", "KC_TRNS", "KC_TRANSPARENT"}
NO_ACTION = {"XXXXXXX", "KC_NO"}

DOC_TAG = re.compile(r"/\*\s*@doc(?:\((.*?)\))?\s*:\s*(.*?)\s*\*/", re.S)
# One whole block comment. Unlike /\*.*?\*/ it cannot run past a */, so a
# search anchored on later code cannot reach back over an earlier comment.
COMMENT = r"/\*[^*]*\*+(?:[^/*][^*]*\*+)*/"
BLOCK_COMMENT = re.compile(COMMENT)
LINE_COMMENT = re.compile(r"//[^\n]*")


class CheatsheetError(Exception):
    pass


def tags_in(text):
    """Return {key: label} for the @doc tags in text. The bare `@doc:` tag has key ''."""
    tags = {}
    for key, label in DOC_TAG.findall(text):
        tags[(key or "").strip()] = " ".join(label.split())
    return tags


def strip_comments(text):
    return LINE_COMMENT.sub("", BLOCK_COMMENT.sub(" ", text))


def read(name):
    return (KEYMAP_DIR / name).read_text()


def braced_block(text, opener, path):
    """Return the text between the `{` that follows opener and its matching `}`."""
    start = text.find(opener)
    if start < 0:
        raise CheatsheetError(f"{path}: cannot find `{opener}`")
    start = text.index("{", start + len(opener) - 1)
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : i]
    raise CheatsheetError(f"{path}: `{opener}` has no closing brace")


def split_args(text):
    """Split on the commas that are not inside parentheses."""
    args, depth, current = [], 0, ""
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            args.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        args.append(current.strip())
    return args


def parse_enum_members(body):
    """Return [(name, value, tags)] for an enum body. value is None when it is not a literal."""
    members = []
    value = -1
    # A member takes the tags that trail it and those on the comment lines
    # directly above it.
    above = ""
    for line in body.splitlines():
        member = re.match(r"\s*(\w+)\s*(?:=\s*([^,/]+?))?\s*,?\s*(.*)$", line)
        if not member:
            above += line
            continue
        name, expr, comment = member.groups()
        if expr is None:
            value = None if value is None else value + 1
        else:
            value = int(expr, 0) if re.fullmatch(r"\d+|0x[0-9a-fA-F]+", expr.strip()) else None
        members.append((name, value, tags_in(above + comment)))
        above = ""
    return members


def enum_values(text, first_member, path):
    """Return {name: value} for the anonymous enum whose first member is first_member."""
    match = re.search(r"enum\s*\{\s*" + first_member + r"\b", text)
    if not match:
        raise CheatsheetError(f"{path}: cannot find the enum that starts with {first_member}")
    body = braced_block(text[match.start() :], "enum", path)
    return {name: value for name, value, _ in parse_enum_members(strip_comments(body))}


def function_tags(sources, name):
    """Return the @doc tags in the comments directly above the definition of name."""
    for path, text in sources.items():
        match = re.search(r"^[ \t]*(?:static\s+)?\w+\s+\**" + name + r"\s*\([^)]*\)\s*\{", text, re.M)
        if not match:
            continue
        # Walk back over the comments and blank lines that sit on top of it.
        above = re.search(r"((?:[ \t]*" + COMMENT + r"[ \t]*\n|[ \t]*\n)*)$", text[: match.start()])
        return tags_in(above.group(1))
    raise CheatsheetError(f"cannot find the definition of {name}()")


@dataclass
class Layer:
    key: str  # the index as written in keymaps[], e.g. LAYER_CONTROL
    index: int
    keys: dict
    tags: dict


@dataclass
class Mode:
    name: str
    index: int
    layer: str
    taps: dict  # "left"/"right" -> (label, detail), for each knob tap the mode has
    wheels: dict
    colour: str  # a colour enum name, e.g. RIGHTY_NAV_LAYER_COLOUR


@dataclass
class Keymap:
    keycode_labels: dict = field(default_factory=dict)
    keycode_details: dict = field(default_factory=dict)
    keycode_holds: dict = field(default_factory=dict)
    layers: list = field(default_factory=list)
    modes: list = field(default_factory=list)
    gestures: dict = field(default_factory=dict)
    picker: dict = field(default_factory=dict)
    colours: dict = field(default_factory=dict)  # colour enum name -> CSS colour, or None
    directions: dict = field(default_factory=dict)  # GESTURE_E -> 0, ...

    def layer(self, key):
        for layer in self.layers:
            if layer.key == key:
                return layer
        raise CheatsheetError(f"keymap.c: no layer {key}")


def parse_keycodes(text):
    body = braced_block(text, "enum my_keycodes", "kitlaan.h")
    labels, details, holds = {}, {}, {}
    for name, _, tags in parse_enum_members(body):
        # A key that only acts while held has a hold tag and may have no label.
        if "" not in tags and "hold" not in tags:
            raise CheatsheetError(f"kitlaan.h: {name} has no `/* @doc: ... */` or `/* @doc(hold): ... */` tag")
        labels[name] = tags.get("")
        for key, table in (("detail", details), ("hold", holds)):
            if key in tags:
                table[name] = tags[key]
    return labels, details, holds


def parse_layers(text, layer_indices):
    body = braced_block(text, "keymaps[][MATRIX_ROWS][MATRIX_COLS] =", "keymap.c")
    layers = []
    previous_end = 0
    for match in re.finditer(r"\[\s*(\w+)\s*\]\s*=\s*LAYOUT\s*\(", body):
        key = match.group(1)
        args_start = match.end()
        depth, i = 1, args_start
        while depth:
            depth += {"(": 1, ")": -1}.get(body[i], 0)
            i += 1
        keys = split_args(strip_comments(body[args_start : i - 1]))
        if len(keys) != len(POSITIONS):
            raise CheatsheetError(f"keymap.c: layer {key} has {len(keys)} keys, not {len(POSITIONS)}")
        index = int(key) if key.isdigit() else layer_indices.get(key)
        if index is None:
            raise CheatsheetError(f"keymap.c: layer {key} is not in the layer enum")
        layers.append(Layer(key, index, dict(zip(POSITIONS, keys)), tags_in(body[previous_end : match.start()])))
        previous_end = i
    return layers


def wheel_labels(tags, fallback=None):
    """Return {"left": label, "right": label}. @doc(wheels) names both, and @doc(wheel_left/right) wins."""
    fallback = fallback or {}
    both = tags.get("wheels")
    return {knob: tags.get(f"wheel_{knob}", both) or fallback.get(knob) for knob in ("left", "right")}


def parse_modes(text, sources, mode_indices):
    body = braced_block(text, "const mode_t modes[] =", "keymap.c")
    modes = []
    for name, fields_text in re.findall(r"\[\s*(\w+)\s*\]\s*=\s*\{(.*?)\}", strip_comments(body), re.S):
        fields = dict(re.findall(r"\.(\w+)\s*=\s*(\w+)", fields_text))
        taps = {}
        for knob in ("left", "right"):
            handler = fields.get(f"tap_{knob}")
            if handler is None:
                continue
            tags = function_tags(sources, handler)
            if "" not in tags:
                raise CheatsheetError(f"{handler}() has no `/* @doc: ... */` tag")
            taps[knob] = (tags[""], tags.get("detail"))
        wheels = wheel_labels(function_tags(sources, fields["wheels"]))
        for knob, label in wheels.items():
            if label is None:
                raise CheatsheetError(f"{fields['wheels']}() has no `@doc(wheels)` or `@doc(wheel_{knob})` tag")
        modes.append(Mode(name, mode_indices[name], fields["layer"], taps, wheels, fields["colour"]))
    return modes


def parse_gestures(text):
    gestures = {}
    for number, direction, label in re.findall(r"case\s+(\d+):\s*/\*\s*([\w-]+),\s*(.*?)\.?\s*\*/", text):
        gestures[int(number)] = (direction, label)
    if sorted(gestures) != list(range(8)):
        raise CheatsheetError(f"gestures.c: expected labelled cases 0-7, found {sorted(gestures)}")
    return gestures


def active_define(text, name, path):
    match = re.search(r"^[ \t]*#define\s+" + name + r"\s+(\S+)", text, re.M)
    if not match:
        raise CheatsheetError(f"{path}: {name} is not defined")
    return match.group(1)


def parse_picker(modes_c, config_h, mode_indices):
    """Return {gesture direction name: mode index or None} for the built picker."""
    directions = int(active_define(config_h, "MODE_PICK_DIRECTIONS", "config.h"))
    body = braced_block(modes_c, "mode_pick_cells[NUM_GESTURE_DIRECTIONS] =", "modes.c")
    # Only the #if/#else that MODE_PICK_DIRECTIONS drives is understood.
    picker, taking, stack = {}, True, []
    for line in body.splitlines():
        line = strip_comments(line).strip()
        if not line:
            continue
        if line.startswith("#if"):
            cond = re.fullmatch(r"#if\s+MODE_PICK_DIRECTIONS\s*==\s*(\d+)", line)
            if not cond:
                raise CheatsheetError(f"modes.c: cannot read `{line}` in mode_pick_cells")
            stack.append(taking)
            taking = taking and directions == int(cond.group(1))
        elif line.startswith("#else"):
            taking = stack[-1] and not taking
        elif line.startswith("#endif"):
            taking = stack.pop()
        elif line.startswith("#"):
            raise CheatsheetError(f"modes.c: cannot read `{line}` in mode_pick_cells")
        elif taking:
            for direction, cell in re.findall(r"\[\s*GESTURE_(\w+)\s*\]\s*=\s*(\w+)", line):
                picker[direction] = None if cell == "MODE_NONE" else int(cell)
    return picker


def hsv_sources():
    """Yield (path, text) for the files that may define the HSV_* colours.

    The board's own colours are in its config.h, which is only in the sibling
    clone or, for CI, in the patch that adds the board. The patch's added lines
    start with `+`, which hsv_defines() allows for.
    """
    roots = [USERSPACE / "qmk_firmware"]
    if os.environ.get("QMK_FIRMWARE_ROOT"):
        roots.insert(0, Path(os.environ["QMK_FIRMWARE_ROOT"]))
    paths = [KEYMAP_DIR / "config.h"]
    for root in roots:
        paths += [root / "keyboards/ploopyco/aplus/config.h", root / "quantum/color.h"]
    paths += sorted((USERSPACE / "ci/patches").glob("*.patch"))
    for path in paths:
        if path.is_file():
            yield path, path.read_text()


def hsv_defines():
    defines = {}
    for _, text in hsv_sources():
        for name, h, s, v in re.findall(r"^\+?[ \t]*#define\s+(HSV_\w+)\s+(\d+),\s*(\d+),\s*(\d+)", text, re.M):
            defines.setdefault(name, (int(h), int(s), int(v)))
    return defines


def qmk_hsv_to_rgb(h, s, v):
    """hsv_to_rgb_impl() in quantum/color.c, integer steps and all."""
    region = h * 6 // 255
    remainder = (h * 2 - region * 85) * 3
    p = (v * (255 - s)) >> 8
    q = (v * (255 - ((s * remainder) >> 8))) >> 8
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8
    return {0: (v, t, p), 1: (q, v, p), 2: (p, v, t), 3: (p, q, v), 4: (t, p, v), 5: (v, p, q), 6: (v, t, p)}[region]


def led_colour(h, s, v):
    """Return a CSS colour that looks like the LED does.

    The LED duty cycle is linear light, so gamma-encode it as screens and
    printers expect. Without this, HSV_GESTUREYELLOW prints red-brown. The
    lightness cap keeps pale results such as NAVGREEN legible as text on white.
    """
    rgb = [(c / 255) ** (1 / 2.2) for c in qmk_hsv_to_rgb(h, s, v)]
    hue, light, sat = colorsys.rgb_to_hls(*rgb)
    r, g, b = colorsys.hls_to_rgb(hue, min(light, 0.45), sat)
    return f"#{round(r * 255):02x}{round(g * 255):02x}{round(b * 255):02x}"


def parse_colours(keymap_c, kitlaan_h):
    """Return {colour enum name: CSS colour or None}.

    The enum value indexes my_rgb_layers, which names a segment, which names an
    HSV_* macro. A colour that cannot be found is None, not an error: it only
    costs the page its tint.
    """
    enum = enum_values(kitlaan_h, "RIGHTY_NAV_LAYER_COLOUR", "kitlaan.h")
    enum.pop("LAYER_COLOUR_COUNT", None)  # a count for keymap.c's assertion, not a colour
    order = split_args(strip_comments(re.search(r"RGBLIGHT_LAYERS_LIST\((.*?)\);", keymap_c, re.S).group(1)))
    segments = dict(re.findall(r"(\w+)\[\]\s*=\s*RGBLIGHT_LAYER_SEGMENTS\(\s*\{[^}]*?(HSV_\w+)\s*\}", keymap_c))
    defines = hsv_defines()
    colours = {}
    for name, index in enum.items():
        hsv = defines.get(segments.get(order[index])) if index < len(order) else None
        if hsv is None:
            colours[name] = None
            continue
        colours[name] = led_colour(*hsv)
    return colours


def parse():
    sources = {name: read(name) for name in ("keymap.c", "modes.c", "wheels.c", "gestures.c")}
    kitlaan_h = read("kitlaan.h")
    modes_h = read("modes.h")
    keymap_c = sources["keymap.c"]

    keymap = Keymap()
    keymap.keycode_labels, keymap.keycode_details, keymap.keycode_holds = parse_keycodes(kitlaan_h)
    keymap.layers = parse_layers(keymap_c, enum_values(kitlaan_h, "LAYER_NAV_RIGHT_HANDED", "kitlaan.h"))
    mode_indices = enum_values(modes_h, "MODE_BASE", "modes.h")
    keymap.modes = parse_modes(keymap_c, sources, mode_indices)
    keymap.gestures = parse_gestures(sources["gestures.c"])
    keymap.picker = parse_picker(sources["modes.c"], read("config.h"), mode_indices)
    keymap.directions = enum_values(sources["modes.c"], "GESTURE_E", "modes.c")
    keymap.colours = parse_colours(keymap_c, kitlaan_h)
    return keymap


def key_label(keymap, layer, keycode):
    """Return what a press of keycode on layer does, or None when a press does nothing."""
    if keycode in layer.tags:
        return layer.tags[keycode]
    if keycode in NO_ACTION:
        return None
    if keycode in keymap.keycode_labels:
        return keymap.keycode_labels[keycode]
    if keycode in BUILTIN_LABELS:
        return BUILTIN_LABELS[keycode]
    toggle = re.fullmatch(r"TG\((\w+)\)", keycode)
    if toggle:
        return f"Toggle {layer_title(keymap.layer(toggle.group(1)))}"
    raise CheatsheetError(f"keymap.c: layer {layer.key} has {keycode}, which has no label")


def layer_title(layer):
    if "" not in layer.tags:
        raise CheatsheetError(f"keymap.c: layer {layer.key} has no `/* @doc: ... */` title")
    return layer.tags[""]


def is_empty(layer):
    return all(k in TRANSPARENT for k in layer.keys.values())


@dataclass
class Control:
    """What one physical control does on a page. Knobs also turn, buttons do not."""

    position: str
    press: str = None
    detail: str = None
    hold: str = None
    turn: str = None


@dataclass
class Arrow:
    direction: int  # 0 is east, then clockwise, as the gestures module counts
    label: str
    colour: str = None
    muted: bool = False


@dataclass
class Page:
    title: str
    colour: str
    controls: list = field(default_factory=list)
    arrows: list = field(default_factory=list)
    notes: list = field(default_factory=list)


def layer_controls(keymap, layer, base=None):
    """Return the controls of layer. base is the layer that a transparent key falls through to."""
    controls = []
    for position in POSITIONS:
        keycode = layer.keys[position]
        source = layer
        if keycode in TRANSPARENT:
            if base is None:
                controls.append(Control(position, press="(held)"))
                continue
            source, keycode = base, base.keys[position]
        # A layer's hold tag gives a key a new meaning, so the stock press
        # label no longer applies. Only an explicit layer label survives it.
        hold = source.tags.get(f"{keycode}, hold")
        detail = source.tags.get(f"{keycode}, detail", keymap.keycode_details.get(keycode))
        if hold is not None:
            controls.append(Control(position, source.tags.get(keycode), detail, hold))
            continue
        controls.append(
            Control(
                position,
                key_label(keymap, source, keycode),
                detail,
                keymap.keycode_holds.get(keycode),
            )
        )
    return controls


def build_pages(keymap):
    pages = []
    mode_layers = set()
    for mode in keymap.modes:
        layer = keymap.layer(mode.layer)
        mode_layers.add(layer.key)
        controls = layer_controls(keymap, layer)
        wheels = wheel_labels(layer.tags, mode.wheels)
        for control in controls:
            if control.position.startswith("knob"):
                control.turn = wheels[control.position.removeprefix("knob_")]
            # The knob keycodes' taps belong to the mode: modes.c runs them.
            knob = KNOB_TAPS.get(layer.keys[control.position])
            if knob in mode.taps:
                control.press, control.detail = mode.taps[knob]
        pages.append(Page(layer_title(layer), keymap.colours.get(mode.colour), controls))

    # The colour names below are the ones that keymap.c and modes.c light for
    # these layers; there is no table that links them.
    base = keymap.layer(keymap.modes[0].layer)
    for layer in keymap.layers:
        if layer.key in mode_layers or is_empty(layer) or layer.key == "LAYER_KNOB_HOLD":
            continue
        controls = layer_controls(keymap, layer, base)
        wheels = wheel_labels(layer.tags)
        for control in controls:
            if control.position.startswith("knob"):
                control.turn = wheels[control.position.removeprefix("knob_")]
        colour = keymap.colours.get(layer.key.removeprefix("LAYER_") + "_LAYER_COLOUR")
        pages.append(Page(layer_title(layer), colour, controls))

    gesture_colour = keymap.colours.get("GESTURE_LAYER_COLOUR")
    pages.append(
        Page(
            "Gestures",
            gesture_colour,
            arrows=[Arrow(n, label) for n, (_, label) in sorted(keymap.gestures.items())],
            notes=["Hold the left knob, roll the ball, then let go of the knob."],
        )
    )

    knob_hold = keymap.layer("LAYER_KNOB_HOLD")
    by_index = {m.index: m for m in keymap.modes}
    arrows = []
    for direction, cell in keymap.picker.items():
        if cell is None:
            continue
        # A cell with no mode is legal: mode_set() refuses it and blinks.
        mode = by_index.get(cell)
        if mode is None:
            arrows.append(Arrow(keymap.directions[f"GESTURE_{direction}"], "(free)", muted=True))
        else:
            title = layer_title(keymap.layer(mode.layer))
            arrows.append(Arrow(keymap.directions[f"GESTURE_{direction}"], title, keymap.colours.get(mode.colour)))
    notes = ["Hold the right knob, roll the ball, then let go of the knob.", "Let go without a roll: back to the last mode."]
    held = [c for c in layer_controls(keymap, knob_hold) if c.press not in (None, "(held)")]
    if held:
        notes.append(f"{layer_title(knob_hold)}: " + ", ".join(f"{POSITION_NAMES[c.position]}: {c.press}" for c in held))
    pages.append(Page("Mode picker", keymap.colours.get("MODE_PICK_LAYER_COLOUR"), arrows=arrows, notes=notes))
    return pages


# --- Drawing -----------------------------------------------------------------
#
# One SVG card per page. The board is a plain outline, not Ploopy's artwork.
# Text is wrapped by character count, which is close enough for one sans font.

GREY = "#777777"
INK = "#1a1a1a"
# 560 high so that two rows of cards fit on a letter page.
CARD_W, CARD_H = 440, 560
# The drawing's coordinates leave room for tall top labels; most pages have
# none, so the whole drawing sits higher than its coordinates say.
DRAWING_SHIFT = -20
CX, CY = 220, 300  # ball centre
BALL_R = 52

# x, y, w, h of each button, from the A+ shape: four along the top, one each
# side of the ball, and the two knobs below.
BUTTONS = {
    "tll": (138, 200, 34, 74),
    "tl": (176, 194, 40, 46),
    "tr": (224, 194, 40, 46),
    "trr": (268, 200, 34, 74),
    "bl": (134, 290, 30, 74),
    "br": (276, 290, 30, 74),
}
KNOBS = {"knob_left": (192, 378), "knob_right": (248, 378)}
KNOB_R = 24

# Where each button's label goes: the text anchor point, its alignment, which
# way extra lines grow, and the point on the button that the leader meets.
CALLOUTS = {
    "tll": ((118, 196), "end", "down", (138, 220)),
    "tl": ((160, 168), "middle", "up", (190, 196)),
    "tr": ((280, 168), "middle", "up", (250, 196)),
    "trr": ((322, 196), "start", "down", (302, 220)),
    "bl": ((118, 310), "end", "down", (134, 325)),
    "br": ((322, 310), "start", "down", (306, 325)),
}
KNOB_TEXT = {"knob_left": (120, 448), "knob_right": (320, 448)}

LABEL_SIZE, DETAIL_SIZE = 13, 10


def line_height(size):
    return round(size * 1.2)


def wrap(text, width):
    lines, line = [], ""
    for word in text.split():
        if line and len(line) + 1 + len(word) > width:
            lines.append(line)
            line = word
        else:
            line = f"{line} {word}".strip()
    return lines + [line] if line else lines


def esc(text):
    return html.escape(text, quote=True)


def text_block(x, y, anchor, grow, runs):
    """Draw runs, a list of (text, size, weight, colour, width), as one block of lines.

    For grow "up" the block's last line sits on y, so that a label above a button
    keeps its gap to the button however many lines it takes.
    """
    lines = [(line, size, weight, colour) for text, size, weight, colour, width in runs for line in wrap(text, width)]
    height = sum(line_height(size) for _, size, _, _ in lines)
    if grow == "up":
        y -= height - line_height(lines[0][1]) if lines else 0
    out = []
    for line, size, weight, colour in lines:
        out.append(
            f'<text x="{x}" y="{y}" text-anchor="{anchor}" font-size="{size}" font-weight="{weight}" '
            f'fill="{colour}">{esc(line)}</text>'
        )
        y += line_height(size)
    return out


def board(colour, faded=False):
    stroke = "#c8c8c8" if faded else INK
    out = [
        f'<g fill="#fff" stroke="{stroke}" stroke-width="2">',
        '<rect x="124" y="184" width="192" height="214" rx="34"/>',
    ]
    for x, y, w, h in BUTTONS.values():
        out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="9"/>')
    for x, y in KNOBS.values():
        out.append(f'<circle cx="{x}" cy="{y}" r="{KNOB_R}"/>')
    out.append("</g>")
    out.append(f'<circle cx="{CX}" cy="{CY}" r="{BALL_R}" fill="#fff" stroke="{colour}" stroke-width="7"/>')
    return out


def leader(x1, y1, x2, y2):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{INK}" stroke-width="1.2" stroke-dasharray="2 3"/>'


def draw_controls(page):
    out = board(page.colour)
    for control in page.controls:
        if control.position in CALLOUTS:
            (x, y), anchor, grow, (bx, by) = CALLOUTS[control.position]
            if control.press is None and control.hold is None:
                continue
            runs = [(control.press, LABEL_SIZE, 700, INK, 15)] if control.press else []
            if control.hold:
                runs.append((f"Hold: {control.hold}", LABEL_SIZE, 700, INK, 15))
            if control.detail:
                runs.append((control.detail, DETAIL_SIZE, 400, GREY, 22))
            out += text_block(x, y, anchor, grow, runs)
            lx = {"end": x + 4, "start": x - 4, "middle": x}[anchor]
            out.append(leader(lx, y + (6 if grow == "up" else -4), bx, by))
        else:
            kx, ky = KNOBS[control.position]
            x, y = KNOB_TEXT[control.position]
            runs = []
            for verb, value in (("Turn", control.turn), ("Press", control.press), ("Hold", control.hold)):
                if value:
                    runs.append((f"{verb}:", DETAIL_SIZE, 400, GREY, 30))
                    runs.append((value, LABEL_SIZE, 700, INK, 22))
            if control.detail:
                runs.append((control.detail, DETAIL_SIZE, 400, GREY, 30))
            out += text_block(x, y, "middle", "down", runs)
            out.append(leader(kx + (-14 if kx < CX else 14), ky + 18, x + (20 if x < CX else -20), y - 14))
    return out


def draw_arrows(page):
    out = board("#c8c8c8", faded=True)
    out.append(f'<circle cx="{CX}" cy="{CY}" r="{BALL_R}" fill="#fff" stroke="{page.colour}" stroke-width="7"/>')
    for arrow in page.arrows:
        angle = math.radians(arrow.direction * 45)
        dx, dy = math.cos(angle), math.sin(angle)
        colour = GREY if arrow.muted else (arrow.colour or INK)
        x1, y1 = CX + dx * (BALL_R + 10), CY + dy * (BALL_R + 10)
        x2, y2 = CX + dx * 125, CY + dy * 125
        style = 'stroke-width="3" stroke-dasharray="6 5"' if arrow.muted else 'stroke-width="7" stroke-linecap="round"'
        out.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{colour}" {style}/>')
        # Arrowhead: a triangle on the far end.
        px, py = -dy, dx
        head = [(x2 + dx * 16, y2 + dy * 16), (x2 + px * 10, y2 + py * 10), (x2 - px * 10, y2 - py * 10)]
        out.append(f'<polygon points="{" ".join(f"{a:.1f},{b:.1f}" for a, b in head)}" fill="{colour}"/>')
        lx, ly = CX + dx * 160, CY + dy * 160
        anchor = "middle" if abs(dx) < 0.3 else ("start" if dx > 0 else "end")
        if abs(dx) >= 0.3:
            lx -= dx * 18
        lines = wrap(arrow.label, 12)
        ly += -line_height(LABEL_SIZE) * (len(lines) - 1) / 2 + 5 if abs(dy) < 0.9 else (0 if dy > 0 else -line_height(LABEL_SIZE) * (len(lines) - 1))
        out += text_block(round(lx), round(ly), anchor, "down", [(arrow.label, LABEL_SIZE, 700, colour, 12)])
    return out


def card(page, revision):
    colour = page.colour or GREY
    page = Page(page.title, colour, page.controls, page.arrows, page.notes)
    out = [
        f'<svg class="card" viewBox="0 0 {CARD_W} {CARD_H}" xmlns="http://www.w3.org/2000/svg" '
        'font-family="Helvetica, Arial, sans-serif">',
        f'<rect x="1" y="1" width="{CARD_W - 2}" height="{CARD_H - 2}" rx="10" fill="#fff" stroke="{INK}" stroke-width="2"/>',
        f'<text x="20" y="40" font-size="20" font-weight="800" fill="{INK}">A+</text>',
    ]
    out += text_block(CARD_W - 20, 40, "end", "down", [(page.title, 24, 700, colour, 18)])
    out.append(f'<g transform="translate(0 {DRAWING_SHIFT})">')
    out += draw_arrows(page) if page.arrows else draw_controls(page)
    out.append("</g>")
    y = CARD_H - 20 - line_height(DETAIL_SIZE) * sum(len(wrap(n, 80)) for n in page.notes)
    for note in page.notes:
        for line in wrap(note, 80):
            out.append(f'<text x="20" y="{y}" font-size="{DETAIL_SIZE}" fill="{INK}">{esc(line)}</text>')
            y += line_height(DETAIL_SIZE)
    if revision:
        out.append(
            f'<text x="{CARD_W - 20}" y="{CARD_H - 12}" text-anchor="end" font-size="9" fill="{GREY}">{esc(revision)}</text>'
        )
    out.append("</svg>")
    return "\n".join(out)


STYLE = """
body { margin: 0; padding: 16px; background: #eee; }
main { display: grid; grid-template-columns: repeat(auto-fill, minmax(320px, 440px)); gap: 16px; justify-content: center; }
.card { width: 100%; height: auto; display: block; }
@page { size: letter; margin: 0.35in; }
@media print {
  body { padding: 0; background: #fff; }
  main { grid-template-columns: 1fr 1fr; gap: 0.15in; }
  .card { break-inside: avoid; }
}
"""


def render(pages, revision):
    cards = "\n".join(card(page, revision) for page in pages)
    return (
        '<!doctype html>\n<html lang="en">\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1">\n'
        f"<title>A+ Cheat Sheet</title>\n<style>{STYLE}</style>\n<main>\n{cards}\n</main>\n</html>\n"
    )


def git_revision():
    try:
        out = subprocess.run(
            ["git", "-C", str(KEYMAP_DIR), "describe", "--always", "--dirty"], capture_output=True, text=True, check=True
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    return out.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("-o", "--output", default=DEFAULT_OUTPUT, help=f"default: {DEFAULT_OUTPUT}")
    parser.add_argument("--strict", action="store_true", help="fail on a colour that cannot be found, for CI")
    args = parser.parse_args()
    try:
        keymap = parse()
        pages = build_pages(keymap)
    except CheatsheetError as e:
        sys.exit(f"cheatsheet.py: {e}")
    missing = sorted(name for name, colour in keymap.colours.items() if colour is None)
    if missing:
        print(f"cheatsheet.py: no colour for {', '.join(missing)}; those pages print grey", file=sys.stderr)
        if args.strict:
            sys.exit(1)
    Path(args.output).write_text(render(pages, git_revision()))


if __name__ == "__main__":
    main()
