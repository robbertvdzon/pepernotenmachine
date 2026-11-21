#!/usr/bin/env python3
"""Design study (NOT firmware) for the start-up splash screen, full 160x128, no tiles / toast.

Five concepts, each animated, each showing the two connections (1 = launcher, 2 = switchboard) in its own way.
Timeline of every GIF (loops): both waiting -> launcher connects at 1.6 s -> switchboard connects at 3.0 s -> "ready"
flourish. Title / subtitle are the English ones; pass `nl` as the second argument for the Dutch variant.

    python3 make_splash_preview.py [outdir] [en|nl]

Needs Pillow and the Unscii fonts (UNSCII_DIR = fontfiles folder).
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H, FPS = 160, 128, 12
LOOP = 5.0
T1, T2 = 1.6, 3.0  # the launcher / the switchboard connect

LANG = sys.argv[2] if len(sys.argv) > 2 else "en"
TITLE = "nut-o-matic" if LANG == "en" else "noot-o-maat"
SUB = "peppernut maker" if LANG == "en" else "pepernoten maker"
VERSION = "v1.0"

RED = (200, 40, 30)
DRED = (120, 20, 16)
GOLD = (232, 184, 48)
CREAM = (244, 236, 216)
NUT = (184, 116, 47)
DNUT = (122, 74, 30)
TEAL = (46, 156, 156)
DTEAL = (28, 100, 104)
NAVY = (14, 22, 52)
INK = (24, 22, 28)
GREEN = (46, 204, 113)
GRAY = (84, 88, 100)
WHITE = (255, 255, 255)


def font(name, size):
    for base in (os.environ.get("UNSCII_DIR", ""), "/home/claude/unscii/fontfiles"):
        p = os.path.join(base, name)
        if base and os.path.exists(p):
            return ImageFont.truetype(p, size)
    raise SystemExit("Unscii font not found (set UNSCII_DIR)")


F16, F8 = font("unscii-16.ttf", 16), font("unscii-8.ttf", 8)


def mix(a, b, f):
    f = max(0.0, min(1.0, f))
    return tuple(int(round(a[i] + (b[i] - a[i]) * f)) for i in range(3))


def text(d, xy, s, f, fill):
    d.fontmode = "1"
    d.text(xy, s, font=f, fill=fill)


def text_c(d, cx, y, s, f, fill):
    w = d.textlength(s, font=f)
    text(d, (cx - w / 2, y), s, f, fill)


def mask_of(s, f, scale=1.0, thr=105):
    """Pixel-font text as a hard mask, scaled up smoothly then thresholded (keeps the letters round and even)."""
    w = int(f.getlength(s)) + 2
    m = Image.new("L", (w, 16), 0)
    dm = ImageDraw.Draw(m)
    dm.fontmode = "1"
    dm.text((0, 0), s, font=f, fill=255)
    if scale != 1.0:
        m = m.resize((int(m.width * scale), int(m.height * scale)), Image.BICUBIC)
    return m.point(lambda v: 255 if v > thr else 0)


LOGO_SCALE = 1.6  # 8x16 font -> ~13x26 px letters: "nut-o-matic" is ~141 px wide


def bold_text(img, cx, y, s, fill, outline=None, shadow=None, stretch=LOGO_SCALE):
    """The logo lettering: smooth enlarged pixel font with a dark outline (and an optional drop shadow)."""
    m = mask_of(s, F16, stretch)
    x = int(cx - m.width / 2)
    om = m.filter(ImageFilter.MaxFilter(3))
    if shadow:
        img.paste(Image.new("RGB", om.size, shadow), (x + 1, y + 2), om)
    if outline:
        img.paste(Image.new("RGB", om.size, outline), (x, y), om)
    img.paste(Image.new("RGB", m.size, fill), (x, y), m)
    return x, m.width


def letters(img, cx, y, s, colors, outline, stretch=LOGO_SCALE):
    """Same lettering, every letter with its own color (the title 'powers up')."""
    cw = 8 * stretch
    x0 = cx - cw * len(s) / 2
    for i, ch in enumerate(s):
        m = mask_of(ch, F16, stretch)
        x = int(x0 + i * cw)
        om = m.filter(ImageFilter.MaxFilter(3))
        img.paste(Image.new("RGB", om.size, outline), (x, y), om)
        img.paste(Image.new("RGB", m.size, colors[i]), (x, y), m)


def nut(d, x, y, r=3, rot=0.0, shade=1.0):
    c = tuple(int(v * shade) for v in NUT)
    dc = tuple(int(v * shade) for v in DNUT)
    d.ellipse((x - r, y - r, x + r, y + r), fill=c)
    for k in range(3):
        a = rot + k * 2.1
        d.point((x + int(round(math.cos(a) * r * 0.5)), y + int(round(math.sin(a) * r * 0.5))), fill=dc)
    d.point((x - r // 2, y - r // 2), fill=mix(c, WHITE, 0.35))


def bt_icon(d, cx, cy, col):
    pts = [(-3, -3), (3, 3), (0, 6), (0, -6), (3, -3), (-3, 3)]
    for i in range(len(pts) - 1):
        d.line((cx + pts[i][0], cy + pts[i][1], cx + pts[i + 1][0], cy + pts[i + 1][1]), fill=col)


def state(t):
    """(launcher connected, switchboard connected, ready)"""
    return t >= T1, t >= T2, t >= T2 + 0.2


def badge(d, x, y, n, on, t, dark=False):
    """Rounded badge with the Bluetooth rune and the device number; pulses while waiting, green when connected."""
    pulse = 0.5 + 0.5 * math.sin(t * 6 + n)
    bg = GREEN if on else mix(GRAY, (150, 156, 170), pulse * 0.6)
    d.rounded_rectangle((x, y, x + 15, y + 13), 3, fill=bg, outline=INK if not dark else None)
    bt_icon(d, x + 8, y + 7, WHITE if on else INK)
    text(d, (x - 8, y + 3), str(n), F8, INK if not dark else CREAM)


def waves(d, x0, x1, y, t, on):
    """Signal arcs between the two badges: travelling while waiting, steady green when both are connected."""
    n = 3
    for i in range(n):
        p = (t * 1.6 + i / n) % 1.0
        cx = x0 + (x1 - x0) * (0.2 + 0.6 * p)
        col = GREEN if on else mix(INK, (200, 200, 210), 1 - abs(p - 0.5) * 2)
        d.arc((cx - 3, y - 4, cx + 3, y + 4), -60, 60, fill=col)


def dots(t):
    return "." * (1 + int(t * 3) % 3)


def status_line(d, cx, y, t, fill, ok_fill=GREEN):
    l1, l2, ready = state(t)
    if ready:
        text_c(d, cx, y, "Ready!", F8, ok_fill)
    else:
        text_c(d, cx, y, "Connecting" + dots(t), F8, fill)


def badge_row(d, y, t, fg, xs=(46, 98), waves_x=(62, 98)):
    l1, l2, ready = state(t)
    badge(d, xs[0], y, 1, l1, t, dark=fg != INK)
    badge(d, xs[1], y, 2, l2, t, dark=fg != INK)
    waves(d, xs[0] + 16, xs[1], y + 7, t, ready)


# ------------------------------------------------------------------ A: Salvo (closest to the example)
def sint(d, x, y):
    d.line((x + 2, y + 6, x + 2, y + 54), fill=GOLD)  # staff
    d.ellipse((x - 1, y + 2, x + 6, y + 9), outline=GOLD)
    d.polygon([(x + 22, y + 26), (x + 32, y + 18), (x + 34, y + 21), (x + 25, y + 31)], fill=RED)  # raised arm
    d.rectangle((x + 33, y + 17, x + 35, y + 20), fill=WHITE)
    d.rectangle((x + 8, y + 28, x + 26, y + 54), fill=RED)  # robe
    d.rectangle((x + 8, y + 50, x + 26, y + 54), fill=GOLD)
    d.line((x + 17, y + 30, x + 17, y + 50), fill=GOLD)
    d.polygon([(x + 9, y + 16), (x + 25, y + 16), (x + 22, y + 40), (x + 17, y + 46), (x + 12, y + 40)], fill=WHITE)  # beard
    d.rectangle((x + 11, y + 12, x + 23, y + 20), fill=(240, 196, 160))  # face
    d.rectangle((x + 11, y + 15, x + 23, y + 18), fill=WHITE)
    d.point((x + 14, y + 14), fill=INK)
    d.point((x + 20, y + 14), fill=INK)
    d.polygon([(x + 17, y - 4), (x + 25, y + 12), (x + 9, y + 12)], fill=RED)  # mitre
    d.line((x + 17, y, x + 17, y + 9), fill=GOLD)
    d.line((x + 13, y + 5, x + 21, y + 5), fill=GOLD)
    d.line((x + 9, y + 11, x + 25, y + 11), fill=GOLD)


def concept_a(t):
    img = Image.new("RGB", (W, H), CREAM)
    d = ImageDraw.Draw(img)
    sint(d, 6, 4)
    d.rectangle((56, 38, 94, 56), fill=TEAL)  # the machine
    d.rectangle((56, 52, 94, 56), fill=DTEAL)
    d.ellipse((64, 42, 74, 52), fill=GOLD, outline=DTEAL)
    d.polygon([(86, 38), (92, 30), (110, 14), (120, 22), (102, 36), (94, 44)], fill=TEAL, outline=DTEAL)  # trumpet barrel
    d.ellipse((106, 12, 122, 26), fill=INK)
    rec = 2 if (t % 1.2) < 0.12 else 0
    d.rectangle((52 + rec, 56, 98 + rec, 59), fill=GOLD)
    for i in range(7):  # nuts flying in arcs
        p = (t / 1.5 + i / 7.0) % 1.0
        x = 112 + p * 70
        y = 20 - 46 * p + 74 * p * p
        nut(d, int(x), int(y), 3 if i % 2 else 2, t * 6 + i)
    if (t % 1.2) < 0.35:  # smoke
        q = (t % 1.2) / 0.35
        for k in range(3):
            r = 2 + q * 4 + k
            c = mix(WHITE, CREAM, q)
            d.ellipse((118 + k * 3 - r, 12 - k * 2 - q * 6 - r, 118 + k * 3 + r, 12 - k * 2 - q * 6 + r), fill=c, outline=(200, 200, 210))
    bold_text(img, 80, 62, TITLE, RED, outline=DRED)
    text_c(d, 80, 90, SUB, F8, INK)
    badge_row(d, 101, t, INK)
    status_line(d, 80, 118, t, INK)
    text(d, (128, 2), VERSION, F8, mix(CREAM, INK, 0.45))
    l1, l2, ready = state(t)
    if ready:  # burst of nuts out of the cannon
        q = min(1.0, (t - (T2 + 0.2)) / 1.2)
        for i in range(10):
            ang = -0.35 - i * 0.12
            nut(d, int(112 + math.cos(ang) * 60 * q), int(20 + math.sin(ang) * 60 * q + 70 * q * q), 3, i + t * 5)
    return img


# ------------------------------------------------------------------ B: the automat (two jars fill up)
def jar(d, x, y, w, h, level, on, t, n):
    d.rounded_rectangle((x, y, x + w, y + h), 5, fill=(30, 40, 60), outline=GOLD)
    inner_h = h - 4
    if level > 0:
        top = y + h - 2 - int(inner_h * level)
        d.rectangle((x + 2, top, x + w - 2, y + h - 2), fill=mix(NUT, (60, 30, 12), 0.5))
        count = int((w - 4) * inner_h * level / 22)
        for k in range(count):  # a heap of nuts, scattered
            px = x + 5 + ((k * 2654435761) >> 7) % (w - 10)
            py = top + 2 + ((k * 40503 + n * 977) >> 3) % max(1, int(inner_h * level) - 3)
            nut(d, px, py, 2, k)
    d.line((x + 4, y + 3, x + 4, y + h - 4), fill=(110, 130, 170))  # glass reflection
    badge(d, x + w // 2 - 7, y + h + 4, n, on, t, dark=True)


def concept_b(t):
    img = Image.new("RGB", (W, H), (42, 14, 20))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((4, 2, 155, 44), 6, fill=(60, 18, 26), outline=GOLD)  # marquee plate
    for i in range(40):  # chasing bulbs
        a_ = i / 40.0
        if a_ < 0.3:
            bx, by = 8 + a_ / 0.3 * 144, 4
        elif a_ < 0.5:
            bx, by = 152, 4 + (a_ - 0.3) / 0.2 * 38
        elif a_ < 0.8:
            bx, by = 152 - (a_ - 0.5) / 0.3 * 144, 42
        else:
            bx, by = 8, 42 - (a_ - 0.8) / 0.2 * 38
        on = int(t * 10 + i) % 3 == 0
        d.ellipse((bx - 1, by - 1, bx + 1, by + 1), fill=(255, 235, 140) if on else (110, 80, 40))
    bold_text(img, 80, 7, TITLE, CREAM, outline=RED, shadow=(30, 8, 12))
    text_c(d, 80, 34, SUB, F8, GOLD)
    l1, l2, ready = state(t)

    def level(tc):  # trickles while waiting, fills up when the device is connected
        if t >= tc:
            return min(1.0, 0.3 + (t - tc) * 1.2)
        return 0.08 + 0.1 * (0.5 + 0.5 * math.sin(t * 2)) if t > 0.3 else 0.0

    jar(d, 12, 50, 52, 44, level(T1), l1, t, 1)
    jar(d, 96, 50, 52, 44, level(T2), l2, t, 2)
    d.rectangle((70, 56, 90, 94), fill=(70, 30, 40), outline=GOLD)  # chute, coin slot and tray
    d.ellipse((76, 61, 84, 69), fill=(255, 235, 140) if int(t * 4) % 2 == 0 and not ready else (110, 80, 40))
    d.rectangle((74, 76, 86, 84), fill=INK)
    d.rectangle((68, 90, 92, 94), fill=GOLD)
    if ready:
        q = (t - (T2 + 0.2)) % 1.0
        nut(d, int(78 + q * 10), int(80 + q * 8), 3, q * 8)
    text_c(d, 80, 114, "Ready!" if ready else "Connecting" + dots(t), F8, GREEN if ready else CREAM)
    text(d, (128, 121), VERSION, F8, (150, 105, 105))
    return img


# ------------------------------------------------------------------ D: rooftops at night (the title powers up)
def concept_d(t):
    img = Image.new("RGB", (W, H), NAVY)
    d = ImageDraw.Draw(img)
    for i in range(26):  # stars
        sx, sy = (i * 53) % 158 + 1, (i * 29) % 56 + 1
        if int(t * 3 + i) % 4:
            d.point((sx, sy), fill=mix(NAVY, WHITE, 0.4 + 0.5 * ((i * 7) % 3) / 3))
    d.ellipse((118, 8, 148, 38), fill=CREAM)  # moon with craters
    for cx, cy, r in ((128, 16, 3), (138, 26, 4), (124, 28, 2)):
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=mix(CREAM, (180, 170, 150), 0.5))
    # rooftops
    d.polygon([(0, 128), (0, 100), (30, 84), (60, 100), (60, 128)], fill=(10, 14, 34))
    d.polygon([(50, 128), (50, 104), (86, 88), (122, 104), (122, 128)], fill=(18, 24, 50))
    d.polygon([(110, 128), (110, 98), (140, 82), (160, 94), (160, 128)], fill=(10, 14, 34))
    d.rectangle((72, 78, 80, 94), fill=(18, 24, 50))  # chimney
    d.rectangle((70, 76, 82, 79), fill=(30, 36, 70))
    for wx in (12, 128):  # lit windows
        d.rectangle((wx, 108, wx + 6, 114), fill=GOLD if int(t * 2 + wx) % 5 else (150, 110, 30))
    # Sint on the roof: silhouette of mitre, beard and staff, waving
    sx = 100
    d.polygon([(sx, 70), (sx + 6, 82), (sx - 6, 82)], fill=(8, 10, 24))
    d.rectangle((sx - 5, 82, sx + 5, 96), fill=(8, 10, 24))
    d.line((sx + 9, 74 + int(math.sin(t * 5) * 1), sx + 9, 98), fill=(8, 10, 24))
    d.ellipse((sx + 6, 70, sx + 12, 76), outline=(8, 10, 24))
    # a cannon on the chimney fires nuts to the moon
    for i in range(6):
        p = (t / 1.8 + i / 6.0) % 1.0
        x = 80 + p * 46
        y = 74 - 60 * p + 10 * p * p
        nut(d, int(x), int(y), 2 if i % 2 else 3, t * 5 + i, 1.0 - 0.35 * p)
    if (t % 0.3) < 0.1:
        d.ellipse((76, 70, 84, 77), fill=mix(GOLD, WHITE, 0.4))
    # the title: letters power up as the devices connect
    l1, l2, ready = state(t)
    cols = []
    for i, ch in enumerate(TITLE):
        left = i < 5
        on = l1 if left else l2
        flick = 0.5 + 0.5 * math.sin(t * 9 + i * 1.7)
        dead = mix((40, 50, 90), (70, 80, 130), flick)
        col = RED if on else dead
        if on and ready and int(t * 6 + i) % 9 == 0:
            col = mix(RED, WHITE, 0.6)
        cols.append(col)
    letters(img, 80, 50, TITLE, cols, outline=(6, 8, 22))
    text_c(d, 80, 80, SUB, F8, mix((120, 130, 180), WHITE, 0.4 if ready else 0.0))
    status_line(d, 80, 116, t, (150, 160, 210))
    badge(d, 24, 100, 1, l1, t, dark=True)
    badge(d, 122, 100, 2, l2, t, dark=True)
    text(d, (2, 2), VERSION, F8, (80, 90, 140))
    return img


# ------------------------------------------------------------------ F: arcade boot
def concept_f(t):
    img = Image.new("RGB", (W, H), (6, 6, 14))
    d = ImageDraw.Draw(img)
    l1, l2, ready = state(t)
    blink = int(t * 3) % 2 == 0
    for y, n, on, label in ((3, 1, l1, "LAUNCHER"), (13, 2, l2, "BOARD")):  # HUD: one row per device
        text(d, (4, y), "%dUP" % n, F8, GREEN if on else (RED if blink else DRED))
        text(d, (32, y), label, F8, mix(GRAY, WHITE, 0.5))
        s_ = "READY" if on else "WAIT"
        text(d, (156 - 8 * len(s_), y), s_, F8, GREEN if on else GOLD)
    d.line((0, 24, 160, 24), fill=(60, 60, 110))
    for i in range(-8, 9):  # perspective grid floor
        d.line((80 + i * 4, 74, 80 + i * 22, 128), fill=(40, 40, 110))
    for k in range(6):
        yy = 74 + int((k + (t * 2) % 1.0) ** 1.7 * 2.3)
        if yy < 128:
            d.line((0, yy, 160, yy), fill=(40, 40, 110))
    bx = 80 + int(math.sin(t * 2.4) * 58)  # a nut bounces
    by = 98 - int(abs(math.sin(t * 4.8)) * 22)
    d.ellipse((bx - 5, 101, bx + 5, 104), fill=(20, 20, 50))
    nut(d, bx, by, 5, t * 7)
    bold_text(img, 80, 32, TITLE, GOLD, outline=DRED, shadow=(80, 20, 60))
    text_c(d, 80, 62, SUB, F8, (150, 220, 255))
    msg = "GET READY!" if ready else "INSERT NUT"
    if blink:
        text_c(d, 80, 114, msg, F8, WHITE if ready else GOLD)
    text(d, (2, 120), "(c) " + VERSION, F8, (80, 80, 120))
    for y in range(0, H, 2):  # light CRT scanlines
        for x in range(0, W):
            r, g, b_ = img.getpixel((x, y))
            img.putpixel((x, y), (int(r * 0.78), int(g * 0.78), int(b_ * 0.78)))
    return img


# ------------------------------------------------------------------ M: minimal orbit (clean, logo first)
def concept_m(t):
    img = Image.new("RGB", (W, H), CREAM)
    d = ImageDraw.Draw(img)
    l1, l2, ready = state(t)
    cx, cy = 80, 30  # orbiting nuts around a big one; the lit ones show the connections (left half 1, right half 2)
    for i in range(8):
        a_ = i * math.pi / 4 - math.pi / 2 + t * 1.2
        x, y = cx + math.cos(a_) * 24, cy + math.sin(a_) * 20
        lit = (math.cos(a_) < 0 and l1) or (math.cos(a_) >= 0 and l2)
        r = 4
        if lit:
            d.ellipse((x - r - 1, y - r - 1, x + r + 1, y + r + 1), fill=GREEN)
        nut(d, int(x), int(y), r, a_, 1.0 if lit or not (l1 or l2) else 0.7)
    big = 10 + (1 if ready and int(t * 6) % 2 == 0 else 0)
    nut(d, cx, cy, big, t * 2)
    bold_text(img, 80, 62, TITLE, RED, outline=DRED)
    text_c(d, 80, 90, SUB, F8, INK)
    badge_row(d, 101, t, INK)
    status_line(d, 80, 118, t, INK)
    text(d, (128, 2), VERSION, F8, mix(CREAM, INK, 0.45))
    return img


CONCEPTS = [
    ("A_salvo", "A  Salvo", concept_a),
    ("B_automat", "B  The automat", concept_b),
    ("D_rooftops", "D  Rooftops at night", concept_d),
    ("F_arcade", "F  Arcade boot", concept_f),
    ("M_minimal", "M  Minimal orbit", concept_m),
]


def gif(frames, path, k=3):
    fr = [f.resize((W * k, H * k), Image.NEAREST).quantize(colors=96, method=Image.MEDIANCUT) for f in frames]
    fr[0].save(path, save_all=True, append_images=fr[1:], duration=int(1000 / FPS), loop=0, optimize=False, disposal=1)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "out"
    os.makedirs(out, exist_ok=True)
    n = int(LOOP * FPS)
    sheet_cols = 3
    sheet = Image.new("RGB", (sheet_cols * (W * 2 + 8) + 8, 4 * (H * 2 + 18) + 8), (40, 40, 48))
    sd = ImageDraw.Draw(sheet)
    cell = 0
    for key, label, fn in CONCEPTS:
        frames = [fn(i / FPS) for i in range(n)]
        gif(frames, os.path.join(out, "splash_%s_%s.gif" % (key, LANG)))
        for tt, tag in ((2.2, "connecting"), (4.2, "ready")):
            im = frames[int(tt * FPS)].resize((W * 2, H * 2), Image.NEAREST)
            x = 8 + (cell % sheet_cols) * (W * 2 + 8)
            y = 8 + (cell // sheet_cols) * (H * 2 + 18)
            sheet.paste(im, (x, y + 12))
            text(sd, (x, y + 1), "%s (%s)" % (label, tag), F8, (230, 230, 235))
            cell += 1
    sheet.crop((0, 0, sheet.width, 8 + ((cell + sheet_cols - 1) // sheet_cols) * (H * 2 + 18))).save(os.path.join(out, "splash_concepts_%s.png" % LANG))
    print("written to", out)


if __name__ == "__main__":
    main()
