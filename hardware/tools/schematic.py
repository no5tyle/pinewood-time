#!/usr/bin/python3
"""Draws docs/schematic.svg, the board's circuit by block: python3 hardware/tools/schematic.py

The netlist itself lives in hardware/generate_board.py; this is the same circuit drawn for people.
"""
import os

HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(HERE, "docs", "schematic.svg")
INK, MUTED, ACCENT, PANEL = "#1d2530", "#5d6875", "#1f7a45", "#f5f7f8"
el = []


def line(*pts, w=1.6, c=INK):
    el.append(f'<polyline points="{" ".join(f"{x},{y}" for x, y in pts)}" fill="none" stroke="{c}" '
              f'stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"/>')


def txt(x, y, s, size=13, anchor="start", c=INK, weight=400):
    s = s.replace("&", "&amp;").replace("<", "&lt;")
    el.append(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{c}" '
              f'font-weight="{weight}">{s}</text>')


def dot(x, y):
    el.append(f'<circle cx="{x}" cy="{y}" r="3.2" fill="{INK}"/>')


def res(x1, y1, x2, y2, label, value):
    """A resistor (IEC box) between two points on a horizontal or vertical line"""
    if y1 == y2:
        m = (x1 + x2) / 2
        line((x1, y1), (m - 22, y1))
        line((m + 22, y1), (x2, y2))
        el.append(f'<rect x="{m - 22}" y="{y1 - 8}" width="44" height="16" rx="2" fill="#fff" stroke="{INK}" stroke-width="1.6"/>')
        txt(m, y1 - 14, f"{label} {value}", 12, "middle")
    else:
        m = (y1 + y2) / 2
        line((x1, y1), (x1, m - 22))
        line((x1, m + 22), (x2, y2))
        el.append(f'<rect x="{x1 - 8}" y="{m - 22}" width="16" height="44" rx="2" fill="#fff" stroke="{INK}" stroke-width="1.6"/>')
        txt(x1 + 14, m + 4, f"{label} {value}", 12)


def rail(x, y, name):
    line((x, y), (x, y - 12))
    line((x - 10, y - 12), (x + 10, y - 12), w=2)
    txt(x, y - 18, name, 11, "middle", ACCENT, 600)


def gnd(x, y):
    line((x, y), (x, y + 10))
    line((x - 10, y + 10), (x + 10, y + 10), w=2)
    line((x - 6, y + 14), (x + 6, y + 14), w=1.6)
    line((x - 2, y + 18), (x + 2, y + 18), w=1.4)


def pin(x, y, label, side="left"):
    """A connector pin: small open circle with its name"""
    el.append(f'<circle cx="{x}" cy="{y}" r="4" fill="#fff" stroke="{INK}" stroke-width="1.6"/>')
    if side == "left":
        txt(x - 9, y + 4, label, 12, "end", MUTED)
    else:
        txt(x + 9, y + 4, label, 12, "start", MUTED)


def gpio(x, y, name):
    el.append(f'<rect x="{x}" y="{y - 11}" width="74" height="22" rx="11" fill="{ACCENT}"/>')
    txt(x + 37, y + 4, name, 12, "middle", "#fff", 600)


def panel(x, y, w, h, title, sub=""):
    el.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="{PANEL}" stroke="#dde2e6"/>')
    txt(x + 16, y + 26, title, 15, weight=700)
    if sub:
        txt(x + 16, y + 44, sub, 12, c=MUTED)


# ---- Lane input, x8
panel(20, 20, 560, 300, "Lane sensor input  (x8, lane n = 1..8)", "Jn: JST XH 4-pin, lane n to GP(n-1)")
px = 80
for k, (name, y) in enumerate([("1 LED+", 110), ("2 SIG", 170), ("3 GND", 230), ("4 5V", 280)]):
    pin(px, y, name)
txt(px, 92, "Jn", 13, "middle", weight=700)
# LED+ via 180 from 5 V
res(px + 4, 110, 300, 110, "Rn", "180")
line((300, 110), (300, 92)); rail(300, 92, "5V")
# SIG: pull-up to 3V3, series 1k to GPIO
line((px + 4, 170), (330, 170)); dot(330, 170)
res(330, 170, 330, 92, "R(10+n)", "10k"); rail(330, 92, "3V3")
res(330, 170, 470, 170, "R(20+n)", "1k")
gpio(472, 170, "GP(n-1)")
line((px + 4, 230), (140, 230)); gnd(140, 230)
line((px + 4, 280), (200, 280), (200, 262)); rail(200, 262, "5V")
txt(36, 312, "A break-beam receiver or phototransistor switches SIG to GND as a car passes.", 11, c=MUTED)

# ---- Start gate and reset
panel(600, 20, 580, 300, "Start gate and reset", "Switches to GND")
for k, (j, g, nm, r1, r2) in enumerate([("J9 GATE", "GP18", "gate switch", "R31", "R32"),
                                         ("J10 RESET", "GP19", "MX key", "R33", "R34")]):
    y = 130 + k * 110
    pin(680, y, "1"); pin(680, y + 30, "2")
    txt(680, y - 22, j, 13, "middle", weight=700)
    line((684, y + 30), (720, y + 30)); gnd(720, y + 30)
    line((684, y), (820, y)); dot(820, y)
    res(820, y, 820, y - 60 if k == 0 else y - 50, r1, "10k")
    rail(820, y - 60 if k == 0 else y - 50, "3V3")
    res(820, y, 990, y, r2, "1k")
    gpio(992, y, g)
# ---- Status LED
panel(20, 340, 370, 260, "Status light", "D1: 5 mm RGB, common cathode")
for k, (g, r, c) in enumerate([("GP20", "R37", "R"), ("GP21", "R38", "G"), ("GP22", "R39", "B")]):
    y = 410 + k * 50
    gpio(40, y, g)
    res(114, y, 270, y, r, "220")
    el.append(f'<polygon points="270,{y - 9} 270,{y + 9} 288,{y}" fill="#fff" stroke="{INK}" stroke-width="1.6"/>')
    line((288, y - 9), (288, y + 9))
    txt(278, y + 24, c, 10, "middle", MUTED)
    line((288, y), (320, y))
line((320, 410), (320, 520)); dot(320, 460); gnd(320, 520)
txt(36, 590, "amber: waiting · green: ready · blue: racing · red: done", 11, c=MUTED)

# ---- Displays (I2C)
panel(410, 340, 370, 260, "Lane displays (I2C)", "J11: GND 3V3 SCL SDA")
for k, (g, n) in enumerate([("GP17", "SCL"), ("GP16", "SDA")]):
    y = 450 + k * 60
    gpio(430, y, g)
    line((504, y), (720, y)); pin(724, y, n, "right")
    x = 650 - k * 30
    dot(x, y)
    if k == 0:
        res(x, y, x, 400, "R35", "4.7k"); rail(x, 400, "3V3")
    else:  # below its line, so it doesn't cross SCL
        res(x, y, x, 566, "R36", "4.7k")
        line((x - 10, 566), (x + 10, 566), w=2); txt(x + 16, 570, "3V3", 11, c=ACCENT, weight=600)
txt(426, 590, "HT16K33 backpacks, address 0x70 + lane - 1", 11, c=MUTED)

# ---- Power
panel(800, 340, 380, 260, "Power (from the Pico's USB)", "VBUS (pin 40) = 5V rail; 3V3 OUT (pin 36) = 3V3 rail")
for k, (name, caps) in enumerate([("5V", [("C1", "100µF")]), ("3V3", [("C2", "100nF")])]):
    y = 430 + k * 90
    gpio(820, y, "VBUS" if k == 0 else "3V3 OUT")
    line((894, y), (1150, y))
    txt(1150, y - 8, name + " rail", 12, "end", ACCENT, 600)
    for j, (c, v) in enumerate(caps):
        x = 950 + j * 90
        dot(x, y)
        line((x, y), (x, y + 18)); line((x - 12, y + 18), (x + 12, y + 18), w=2); line((x - 12, y + 24), (x + 12, y + 24), w=2)
        line((x, y + 24), (x, y + 34)); gnd(x, y + 34)
        txt(x + 16, y + 26, f"{c} {v}", 12)
txt(816, 590, "J12 (AUX): GND, 3V3, 5V, GP26, GP27, GP28 for extras.", 11, c=MUTED)

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 620" width="1200" height="620" '
       f'font-family="Inter, -apple-system, Segoe UI, Roboto, sans-serif">'
       f'<rect width="1200" height="620" fill="#ffffff"/>' + "".join(el) + "</svg>")
os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, "w").write(svg)
print(OUT)
