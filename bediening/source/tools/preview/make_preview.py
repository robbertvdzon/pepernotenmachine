#!/usr/bin/env python3
"""Off-target DESIGN PREVIEW of the Phase 3 home screen (160x128). Not firmware.

Renders the launcher status band (animated per state), the 7 function tiles and the toast
row to PNG / GIF so the look can be judged before any firmware is written.
Needs Pillow and the Unscii fonts (https://github.com/viznut/unscii, fontfiles/unscii-16.ttf
and unscii-8.ttf); point UNSCII_DIR at the fontfiles folder.

    python3 make_preview.py [outdir]
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 160, 128
FPS = 12

# Palette = include/ui_theme.h
BG = (0, 0, 0)
CARD = (0x26, 0x26, 0x26)
TEXT = (255, 255, 255)
MUTED = (0x8A, 0x94, 0xA6)
DIM = (0xC4, 0xCA, 0xD8)
ORANGE = (0xF2, 0x8C, 0x28)   # launcher
TEAL = (0x26, 0xA6, 0x9A)     # switchboard
OK = (0x2E, 0xCC, 0x71)
WARN = (0xF5, 0xA6, 0x23)
ERR = (0xE7, 0x4C, 0x3C)
OFFGRAY = (0x5A, 0x60, 0x70)
SMOKE = (225, 230, 240)

# Layout (px)
BAND = (0, 0, 160, 78)
TILE_Y, TILE_W, TILE_H, TILE_GAP, TILE_X0 = 82, 20, 24, 2, 4
TOAST_Y, TOAST_H = 109, 18


def find_font(name, size):
    for base in (os.environ.get("UNSCII_DIR", ""), "/home/claude/unscii/fontfiles",
                 os.path.join(os.path.dirname(os.path.abspath(__file__)), "unscii", "fontfiles")):
        p = os.path.join(base, name)
        if base and os.path.exists(p):
            return ImageFont.truetype(p, size)
    raise SystemExit("Unscii font %s not found (set UNSCII_DIR)" % name)


F16 = find_font("unscii-16.ttf", 16)
F8 = find_font("unscii-8.ttf", 8)


def mix(a, b, f):
    f = max(0.0, min(1.0, f))
    return tuple(int(round(a[i] + (b[i] - a[i]) * f)) for i in range(3))


def text(d, xy, s, font, fill):
    d.fontmode = "1"
    d.text(xy, s, font=font, fill=fill)


def big_text(img, xy, s, scale, fill):
    """Text drawn at 8x16 and scaled up (nearest) for the big digits / question mark."""
    tmp = Image.new("L", (8 * len(s), 16), 0)
    td = ImageDraw.Draw(tmp)
    td.fontmode = "1"
    td.text((0, 0), s, font=F16, fill=255)
    tmp = tmp.resize((tmp.width * scale, tmp.height * scale), Image.NEAREST)
    img.paste(Image.new("RGB", tmp.size, fill), xy, tmp.point(lambda v: 255 if v > 127 else 0))


def puff(d, x, y, r, alpha, color=SMOKE, bg=CARD):
    if alpha <= 0.02:
        return
    c = mix(bg, color, alpha)
    d.ellipse((x - r, y - r, x + r, y + r), fill=c)


# ------------------------------------------------------------------ band sprites
CX = 38  # horizontal centre of the sprite area


def rocket(d, cx, top):
    body, nose, fin, win = (232, 236, 244), (231, 76, 60), (231, 76, 60), (60, 140, 220)
    d.polygon([(cx, top), (cx + 8, top + 12), (cx + 9, top + 30), (cx - 9, top + 30), (cx - 8, top + 12)], fill=body)
    d.polygon([(cx, top), (cx + 6, top + 9), (cx - 6, top + 9)], fill=nose)
    d.polygon([(cx - 9, top + 22), (cx - 16, top + 34), (cx - 9, top + 32)], fill=fin)
    d.polygon([(cx + 9, top + 22), (cx + 16, top + 34), (cx + 9, top + 32)], fill=fin)
    d.ellipse((cx - 4, top + 12, cx + 4, top + 20), fill=win, outline=(150, 170, 200))
    d.rectangle((cx - 4, top + 30, cx + 4, top + 34), fill=(110, 115, 125))
    d.line((cx - 3, top + 14, cx - 2, top + 15), fill=(190, 225, 255))


def pad(d, cx, y):
    d.rectangle((cx - 20, y, cx + 20, y + 4), fill=(120, 126, 138))
    d.rectangle((cx - 14, y + 5, cx - 11, y + 8), fill=(80, 85, 95))
    d.rectangle((cx + 11, y + 5, cx + 14, y + 8), fill=(80, 85, 95))


def chevrons(d, t, direction, color):
    spacing, speed = 22, 34.0
    off = (t * speed) % spacing
    for k in range(-1, 4):
        y0 = 4 + k * spacing + (off if direction > 0 else -off) + (0 if direction > 0 else spacing)
        pts = [(CX - 18, 0), (CX, 18), (CX + 18, 0), (CX + 18, 8), (CX, 26), (CX - 18, 8)]
        if direction < 0:
            pts = [(x, 26 - y) for x, y in pts]
        ymid = y0 + 13
        f = 1.0 - abs(ymid - 39) / 36.0
        if f <= 0.05:
            continue
        d.polygon([(x, y + y0) for x, y in pts], fill=mix(CARD, color, f))


def lock_anim(d, t, color):
    u = t % 1.7
    lift = 9 if u < 0.5 else max(0.0, 9 * (1 - (u - 0.5) / 0.35)) if u < 0.85 else 0
    click = 0.85 <= u < 1.15
    bx0, by0, bx1, by1 = CX - 14, 38, CX + 14, 62
    body = mix(color, TEXT, 0.5) if click else color
    # shackle
    top = 14 - lift
    d.rectangle((CX - 9, top, CX - 5, 40), fill=DIM)
    d.rectangle((CX + 5, top, CX + 9, 40 - (lift * 1.2 if lift > 0 else 0)), fill=DIM)
    d.rectangle((CX - 9, top, CX + 9, top + 4), fill=DIM)
    d.rectangle((bx0, by0, bx1, by1), fill=body)
    d.rectangle((CX - 2, 46, CX + 2, 51), fill=CARD)
    d.rectangle((CX - 1, 51, CX + 1, 56), fill=CARD)
    if click:
        for ang in range(0, 360, 45):
            r0, r1 = 20, 26 + (u - 0.85) * 12
            a = math.radians(ang)
            d.line((CX + r0 * math.cos(a), 50 + r0 * math.sin(a), CX + r1 * math.cos(a), 50 + r1 * math.sin(a)), fill=WARN)


def spinner(d, t, color):
    for i in range(8):
        a = math.radians(i * 45 - 90)
        x, y = CX + 18 * math.cos(a), 39 + 18 * math.sin(a)
        f = ((int(t * 10) - i) % 8) / 8.0
        d.rectangle((x - 3, y - 3, x + 3, y + 3), fill=mix(CARD, color, 1 - f))


def offline_anim(d, t):
    d.line((CX, 30, CX, 56), fill=OFFGRAY, width=3)
    d.polygon([(CX - 10, 60), (CX, 36), (CX + 10, 60)], outline=OFFGRAY)
    d.ellipse((CX - 4, 24, CX + 4, 32), fill=OFFGRAY)
    for k in range(3):
        p = ((t / 1.5) + k / 3.0) % 1.0
        r = 8 + p * 22
        col = mix(CARD, MUTED, 1 - p)
        d.arc((CX - r, 28 - r, CX + r, 28 + r), 200, 340, fill=col, width=2)


def check_anim(img, t):
    f = 0.55 + 0.45 * (0.5 + 0.5 * math.sin(t * 5))
    big_text(img, (CX - 12, 14), "?", 3, mix(CARD, WARN, f))


def error_anim(d, t):
    on = int(t * 3) % 2 == 0
    col = ERR if on else mix(CARD, ERR, 0.35)
    d.polygon([(CX, 8), (CX + 28, 58), (CX - 28, 58)], outline=col, fill=None)
    d.polygon([(CX, 13), (CX + 24, 55), (CX - 24, 55)], outline=col)
    d.polygon([(CX, 18), (CX + 20, 52), (CX - 20, 52)], outline=col)
    d.rectangle((CX - 2, 26, CX + 2, 42), fill=col)
    d.rectangle((CX - 2, 46, CX + 2, 50), fill=col)


def ready_anim(d, t):
    pad(d, CX, 58)
    rocket(d, CX, 16)
    for i in range(8):
        p = ((t / 1.9) + i / 8.0) % 1.0
        side = -1 if i % 2 else 1
        x = CX + side * (3 + p * 20 + (i * 5) % 4)
        y = 56 - p * 15 - (i * 3) % 3
        puff(d, x, y, 2 + p * 4, (1 - p) ** 0.6)


def launched_anim(d, t):
    a = 170.0
    top = 16 - 0.5 * a * t * t
    pad(d, CX, 58)
    # ground cloud grows and fades
    for i in range(6):
        side = -1 if i % 2 else 1
        p = min(1.0, t / 1.4)
        puff(d, CX + side * (4 + p * 24 + i * 2), 56 - (i % 3) * 3, 3 + p * 5, (1 - p) ** 0.6)
    # trail
    for k in range(1, 10):
        tk = t - 0.055 * k
        if tk < 0:
            continue
        yk = 16 - 0.5 * a * tk * tk + 36
        puff(d, CX + (k % 3 - 1) * 2, yk, 2 + k * 0.7, max(0.0, 0.85 - k * 0.08))
    if top + 34 > -4:
        fl = 8 + (int(t * 24) % 3) * 3
        d.polygon([(CX - 5, top + 34), (CX + 5, top + 34), (CX, top + 34 + fl + 6)], fill=(231, 76, 60))
        d.polygon([(CX - 3, top + 34), (CX + 3, top + 34), (CX, top + 34 + fl)], fill=(250, 210, 80))
        rocket(d, CX, top)


def rearm_anim(img, d, t, secs_left):
    pad(d, CX, 58)
    for i in range(4):
        d.rectangle((CX - 17 + i * 10, 69, CX - 11 + i * 10, 73), fill=WARN if i < secs_left else mix(CARD, WARN, 0.2))
    big_text(img, (CX - 12, 10), str(max(0, secs_left)), 3, WARN)


# ------------------------------------------------------------------ band
STATES = {
    # key: (accent, big text, small text)
    "offline": (OFFGRAY, "OFFLINE", "searching"),
    "check": (WARN, "CHECK", "answer?"),
    "error": (ERR, "ERROR", "fix by hand"),
    "starting": (WARN, "PREPARING", "starting"),
    "down": (WARN, "PREPARING", "lowering"),
    "lock": (WARN, "PREPARING", "locking"),
    "up": (WARN, "PREPARING", "unwinding"),
    "ready": (OK, "READY", "to launch"),
    "launched": (OK, "LAUNCHED", ""),
    "rearm": (WARN, "RE-ARM", "stand clear"),
}


def draw_band(img, state, t, progress=0.0, secs_left=4):
    x0, y0, x1, y1 = BAND
    b = Image.new("RGB", (x1 - x0, y1 - y0), CARD)
    d = ImageDraw.Draw(b)
    accent, big, small = STATES[state]
    if state == "rearm":  # attention: the stripe alternates with red
        accent = WARN if int(t * 4) % 2 == 0 else ERR
    if state == "down":
        chevrons(d, t, +1, accent)
    elif state == "up":
        chevrons(d, t, -1, accent)
    elif state == "lock":
        lock_anim(d, t, accent)
    elif state == "starting":
        spinner(d, t, accent)
    elif state == "offline":
        offline_anim(d, t)
    elif state == "check":
        check_anim(b, t)
    elif state == "error":
        error_anim(d, t)
    elif state == "ready":
        ready_anim(d, t)
    elif state == "launched":
        launched_anim(d, t)
    elif state == "rearm":
        rearm_anim(b, d, t, secs_left)
    d.rectangle((0, 0, 3, b.height - 1), fill=accent)  # left stripe like the old cards
    text(d, (70, 14), big, F16, TEXT if state not in ("offline",) else MUTED)
    if small:
        text(d, (70, 34), small, F16, accent if state in ("ready", "error", "rearm") else MUTED)
    if state in ("starting", "down", "lock", "up"):
        d.rectangle((70, 60, 150, 64), outline=MUTED)
        d.rectangle((71, 61, 71 + int(78 * progress), 63), fill=accent)
    # round the band corners like the other cards
    mask = Image.new("L", b.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, b.width - 1, b.height - 1), 6, fill=255)
    img.paste(b, (x0, y0), mask)


# ------------------------------------------------------------------ tile icons (16x16, local coords)
def icon_fog(d, t, mode):
    c = mode["color"]
    for cx, cy, r in ((5, 10, 3), (9, 8, 4), (12, 11, 3)):
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=c)
    d.rectangle((3, 11, 14, 14), fill=c)
    if mode.get("anim"):
        for i in range(3):
            p = ((t / 1.2) + i / 3.0) % 1.0
            d.rectangle((3 + i * 4, 6 - p * 6, 4 + i * 4, 7 - p * 6), fill=mix(CARD, c, 1 - p))


def icon_panic(d, t, mode):
    c = mode["color"]
    bell = Image.new("RGB", (16, 16), CARD)
    bd = ImageDraw.Draw(bell)
    bd.rectangle((7, 0, 8, 1), fill=c)
    bd.pieslice((3, 1, 12, 15), 180, 360, fill=c)
    bd.rectangle((3, 8, 12, 10), fill=c)
    bd.rectangle((1, 10, 14, 12), fill=c)
    bd.rectangle((7, 13, 8, 14), fill=c)
    ang = math.sin(t * 9) * 22 if mode.get("anim") else 0
    bell = bell.rotate(ang, resample=Image.NEAREST, center=(8, 1), fillcolor=CARD)
    d._image.paste(bell, (0, 0))


def icon_flash(d, t, mode):
    c = mode["color"]
    d.rectangle((2, 12, 13, 14), fill=c)
    d.pieslice((3, 3, 12, 21), 180, 360, fill=c)
    d.line((6, 6, 6, 10), fill=CARD)  # highlight
    if mode.get("anim"):
        if int(t * 4) % 2:
            rays = ((0, 8, 1, 8), (14, 8, 15, 8), (2, 3, 3, 4), (12, 3, 13, 4))
        else:
            rays = ((0, 5, 1, 6), (14, 5, 15, 6), (7, 0, 7, 1))
        for x0, y0, x1, y1 in rays:
            d.line((x0, y0, x1, y1), fill=c)


def icon_lamp(d, t, mode):
    c = mode["color"]
    d.ellipse((3, 2, 12, 11), fill=c)
    d.rectangle((6, 11, 9, 14), fill=c if not mode.get("anim") else DIM)
    if mode.get("anim") and int(t * 3) % 2 == 0:
        for p in ((0, 3), (15, 3), (1, 10), (14, 10)):
            d.point(p, fill=c)


def icon_siren(d, t, mode):
    c = mode["color"]
    d.polygon([(1, 6), (5, 6), (10, 2), (10, 14), (5, 10), (1, 10)], fill=c)
    if mode.get("anim"):
        n = int(t * 6) % 3
        for k in range(n + 1):
            d.arc((8 + k * 2, 8 - 3 - k * 2, 14 + k * 2, 8 + 3 + k * 2), 300, 60, fill=c)


def icon_auto(d, t, mode):
    c = mode["color"]
    d.ellipse((1, 1, 14, 14), outline=c)
    text(d, (4, 4), "A", F8, c)


def icon_dispenser(d, t, mode):
    c = mode["color"]
    d.polygon([(2, 1), (14, 1), (9, 8), (9, 10), (7, 10), (7, 8)], fill=c)
    if mode.get("anim"):
        for i in range(2):
            p = ((t / 0.6) + i / 2.0) % 1.0
            d.rectangle((7, 11 + p * 5, 8, 12 + p * 5), fill=mix(CARD, c, 1 - p * 0.6))


ICONS = [("fog", icon_fog), ("panic", icon_panic), ("flash", icon_flash), ("lamp", icon_lamp),
         ("siren", icon_siren), ("auto", icon_auto), ("dispenser", icon_dispenser)]
ON_COLOR = {"fog": TEAL, "panic": TEAL, "flash": TEAL, "lamp": TEAL, "siren": TEAL, "auto": TEAL, "dispenser": ORANGE}


def tile_mode(name, st, t):
    """st: off | on | active | pending | problem"""
    if st == "off":
        return {"color": OFFGRAY}
    if st == "pending":  # wanted, not confirmed: amber blink
        return {"color": WARN if int(t * 2) % 2 == 0 else mix(CARD, WARN, 0.3)}
    if st == "problem":
        return {"color": ERR if int(t * 5) % 2 == 0 else mix(CARD, ERR, 0.4)}
    return {"color": ON_COLOR[name], "anim": st == "active"}


def draw_tiles(img, tiles, t):
    for i, (name, fn) in enumerate(ICONS):
        x = TILE_X0 + i * (TILE_W + TILE_GAP)
        d = ImageDraw.Draw(img)
        d.rounded_rectangle((x, TILE_Y, x + TILE_W - 1, TILE_Y + TILE_H - 1), 4, fill=CARD)
        st = tiles.get(name, "off")
        mode = tile_mode(name, st, t)
        ic = Image.new("RGB", (16, 16), CARD)
        fn(ImageDraw.Draw(ic), t, mode)
        img.paste(ic, (x + 2, TILE_Y + 4))
        if st in ("problem", "pending"):
            d.rectangle((x + 4, TILE_Y + TILE_H - 3, x + TILE_W - 5, TILE_Y + TILE_H - 2), fill=mode["color"])


def draw_toast(img, toast, t_since):
    if not toast:
        return
    msg, level = toast
    col = ERR if level == "error" else WARN
    slide = max(0.0, 1.0 - t_since / 0.2) * (TOAST_H + 2)
    d = ImageDraw.Draw(img)
    y = int(TOAST_Y + slide)
    d.rounded_rectangle((4, y, 155, y + TOAST_H - 1), 4, fill=mix(CARD, col, 0.35))
    d.rectangle((4, y + 3, 6, y + TOAST_H - 4), fill=col)
    text(d, (11, y + 1), "!", F16, col)
    text(d, (24, y + 1), msg, F16, TEXT)


def compose(state, t, tiles=None, toast=None, toast_t=1.0, progress=0.0, secs_left=4):
    img = Image.new("RGB", (W, H), BG)
    draw_band(img, state, t, progress, secs_left)
    draw_tiles(img, tiles or {}, t)
    draw_toast(img, toast, toast_t)
    return img


# ------------------------------------------------------------------ outputs
def scaled(img, k):
    return img.resize((img.width * k, img.height * k), Image.NEAREST)


def save_gif(frames, path, k=3):
    fr = [scaled(f, k).quantize(colors=64, method=Image.MEDIANCUT) for f in frames]
    fr[0].save(path, save_all=True, append_images=fr[1:], duration=int(1000 / FPS), loop=0, optimize=False, disposal=1)


def demo_timeline():
    """(state, seconds, progress range). Real timings are much longer; compressed for the preview."""
    return [("ready", 3.0, (1, 1)), ("launched", 1.3, (0, 0)), ("rearm", 4.0, (0, 0)), ("starting", 1.0, (0, 0)),
            ("down", 5.0, (0.0, 0.3)), ("lock", 1.7, (0.3, 0.36)), ("up", 6.0, (0.36, 1.0)), ("ready", 2.0, (1, 1))]


def tiles_at(gt, state):
    c = gt % 20
    tiles = {"fog": "off", "flash": "on", "lamp": "on", "panic": "off", "siren": "off", "auto": "off", "dispenser": "off"}
    tiles["fog"] = "pending" if c < 4 else ("on" if c < 7 else "active" if c < 17 else "off")
    if state in ("lock", "up"):
        tiles["dispenser"] = "active"
    if 9 <= c < 12:
        tiles["siren"] = "active"
    if 13 <= c < 16:
        tiles["panic"] = "active"
    return tiles


def make_demo(path):
    frames, gt = [], 0.0
    for state, dur, (p0, p1) in demo_timeline():
        n = int(dur * FPS)
        for i in range(n):
            t = i / FPS
            prog = p0 + (p1 - p0) * i / max(1, n - 1)
            secs = 4 - int(t) if state == "rearm" else 0
            toast = None
            if state == "down" and 1.0 <= t < 4.0:
                toast = ("not ready yet", "warn")
            if state == "up" and 3.0 <= t:
                toast = ("board offline +1", "error")
            frames.append(compose(state, t, tiles_at(gt, state), toast, (t - 1.0) if state == "down" else (t - 3.0), prog, secs))
            gt += 1.0 / FPS
    save_gif(frames, path)


def make_state_gifs(outdir):
    for state in STATES:
        dur = {"launched": 1.3, "lock": 1.7, "rearm": 4.0}.get(state, 2.5)
        fr = [compose(state, i / FPS, {}, None, 1.0, 0.5, 4 - int(i / FPS)) for i in range(int(dur * FPS))]
        save_gif(fr, os.path.join(outdir, "state_%s.gif" % state), 3)


def make_sheet(path):
    sample = [("offline", 0.4), ("check", 0.3), ("error", 0.0), ("starting", 0.3), ("down", 0.5), ("lock", 0.55),
              ("up", 0.7), ("ready", 0.9), ("launched", 0.45), ("rearm", 0.1)]
    tiles = {"fog": "active", "panic": "on", "flash": "on", "lamp": "on", "siren": "off", "auto": "off", "dispenser": "active"}
    cell_w, cell_h, k = W * 2, H * 2 + 14, 2
    cols = 2
    rows = (len(sample) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w + (cols + 1) * 8, rows * cell_h + (rows + 1) * 4), (30, 30, 36))
    d = ImageDraw.Draw(sheet)
    for i, (s, t) in enumerate(sample):
        toast = ("launcher error", "error") if s == "error" else (("not ready yet", "warn") if s == "down" else None)
        tl = dict(tiles)
        if s in ("down", "ready", "launched", "rearm", "offline", "check", "error", "starting"):
            tl["dispenser"] = "off"
        if s == "error":
            tl["fog"] = "problem"
        im = scaled(compose(s, t, tl, toast, 1.0, 0.5, 3), k)
        x = 8 + (i % cols) * (cell_w + 8)
        y = 4 + (i // cols) * (cell_h + 4)
        sheet.paste(im, (x, y + 14))
        text(d, (x, y + 2), s, F8, (200, 200, 210))
    sheet.save(path)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "out"
    os.makedirs(out, exist_ok=True)
    make_sheet(os.path.join(out, "home_states.png"))
    make_demo(os.path.join(out, "demo_cycle.gif"))
    make_state_gifs(out)
    print("written to", out)
