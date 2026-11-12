#!/usr/bin/env python3
"""Draws renders/sensor-fit.svg: a section across lane 2 at the finish line, to scale (4 px per mm)"""
import os
S = 4.0
OX, OY = 110, 330          # where track x=148-60, z=0 lands
INK, MUTED, ACC = "#1d2530", "#5d6875", "#1f7a45"
el = []
def X(x): return OX + (x - 88) * S
def Z(z): return OY - z * S
def rect(x0, z0, x1, z1, fill, stroke=INK, w=1.2, op=1):
    el.append(f'<rect x="{X(x0)}" y="{Z(z1)}" width="{(x1-x0)*S}" height="{(z1-z0)*S}" fill="{fill}" fill-opacity="{op}" stroke="{stroke}" stroke-width="{w}"/>')
def txt(x, y, s, size=13, anchor="start", c=INK, weight=400):
    el.append(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{c}" font-weight="{weight}">{s}</text>')
def dim_v(x, z0, z1, label, side=1):
    px = X(x)
    el.append(f'<line x1="{px}" y1="{Z(z0)}" x2="{px}" y2="{Z(z1)}" stroke="{ACC}" stroke-width="1.2" marker-start="url(#a)" marker-end="url(#a)"/>')
    txt(px + 8 * side, (Z(z0) + Z(z1)) / 2 + 4, label, 12, "start" if side > 0 else "end", ACC, 600)

# floor; board and rail with the 10 mm hole
rect(88, -32, 208, -28, "#d9d9d9", "#b5b5b5")
for (a, b) in [(88, 143), (153, 208)]:
    rect(a, -9, b, 0, "#e7c99a")
for (a, b) in [(128, 143), (153, 168)]:
    rect(a, 0, b, 6, "#d6ae74")
# the sensor bar (cut): the bar under the board, the turret up the hole, the light bore through both
rect(88, -22, 208, -9, "#f39c12", INK, 1.2, 0.9)
rect(143.2, -9, 152.8, 5, "#f39c12", INK, 1.2, 0.9)
rect(145, -13.7, 151, 5, "#ffffff", "#c77d0a", 1)
# the emitter, lying flat, LED up on the lane centre; its M2 screw; its cable out of the left end
rect(133, -21.7, 153, -13.7, "#2b2b2b", "#000")
el.append(f'<circle cx="{X(148)}" cy="{Z(-13.9)}" r="{1.5*S}" fill="#c0392b"/>')
rect(137.2, -24, 138.8, -12, "#9aa4ad", "#5d6875", 0.8)
rect(135.5, -24.6, 140.5, -22, "#9aa4ad", "#5d6875", 0.8)
el.append(f'<path d="M {X(133)} {Z(-17.7)} L {X(126)} {Z(-17.7)} q {-3*S} 0 {-3*S} {2*S} L {X(123)} {Z(-21)}" fill="none" stroke="#444" stroke-width="3"/>')
# beam up to the gantry
el.append(f'<line x1="{X(148)}" y1="{Z(-13)}" x2="{X(148)}" y2="{Z(60)}" stroke="#c0392b" stroke-width="2" stroke-dasharray="6 5"/>')
txt(X(148) + 10, Z(55), "beam up to the receiver in the gantry, 120 mm above", 12, "start", "#c0392b", 600)
txt(X(122), Z(-12.5), "cable, to the groove", 11, "end", "#5a3a00")
txt(X(134.5), Z(-23.3) + 4, "M2 screw", 11, "end", MUTED)

# dimensions
dim_v(85, -9, 0, "9 board", -1)
dim_v(122, 0, 6, "6 rail", -1)
dim_v(85, -28, -9, "19 to the floor", -1)
dim_v(214, -22, -9, "13 bar", 1)
dim_v(206, -28, -22, "6 clear", 1)
dim_v(158, -21.7, -13.7, "8 sensor", 1)
txt(X(148) - 14, Z(12), "turret top 1 mm below the rail top", 12, "end", INK, 600)
txt(X(135), Z(-9) + 2 * S + 40, "", 1)
# labels
txt(20, 34, "Lane 2 at the finish line, cut across the track (mm, to scale)", 16, "start", INK, 700)
txt(20, 56, "One 10 mm hole through rail and board; the sensor bar (orange) holds the emitter flat underneath, looking up through its turret.", 12, "start", MUTED)
txt(X(110), Z(-3), "track board", 11, "middle", MUTED); txt(X(135), Z(6) - 6, "rail", 11, "middle", MUTED); txt(X(161), Z(6) - 6, "rail", 11, "middle", MUTED)
txt(X(148), Z(-28) + 18, "floor", 11, "middle", MUTED)
W, H = 940, 470
svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Inter, -apple-system, Segoe UI, Roboto, sans-serif">'
       f'<defs><marker id="a" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{ACC}"/></marker></defs>'
       f'<rect width="{W}" height="{H}" fill="#fff"/>' + "".join(el) + "</svg>")
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "renders", "sensor-fit.svg")
open(out, "w").write(svg)
print(out)
