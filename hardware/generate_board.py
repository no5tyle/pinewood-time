#!/usr/bin/python3
"""
Generates the pinewood-time PCB (KiCad 7): a carrier board for a Raspberry Pi Pico that turns it into a
pinewood derby race timer for up to 8 lanes.

    /usr/bin/python3 hardware/generate_board.py place   # places the parts and writes the board
    /usr/bin/python3 hardware/route.py                  # routes every net but GND as tracks
    /usr/bin/python3 hardware/generate_board.py finish  # adds the ground pours and labels

Everything about the circuit is in this file: the parts, where they go and how they're connected (the
netlist). Change it and run hardware/build.sh again. Uses KiCad 7's pcbnew Python module (install KiCad;
run with the system Python that has pcbnew).

All parts are through-hole, so the board can be hand-soldered.
"""
import json
import math
import os
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
BOARD = os.path.join(HERE, "pinewood-time.kicad_pcb")
PROJECT = os.path.join(HERE, "pinewood-time.kicad_pro")
FP = "/usr/share/kicad/footprints"

VERSION = "v1.0"
# Board size, mm. Everything is placed from its top-left corner
W, H = 112.0, 82.0
ORIGIN = (100.0, 60.0)  # where the board sits on KiCad's page

LANES = 8


def mm(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(ORIGIN[0] + x), pcbnew.FromMM(ORIGIN[1] + y))


# ------------------------------------------------------------------------------------------------ Netlist
# Each part: reference, library, footprint, value, position (x, y, rotation) and its pads' nets.
# Per lane n (JST-XH 4-pin socket Jn): 1 = IR LED + (5 V through Rled), 2 = sensor signal, 3 = GND,
# 4 = 5 V (for powered break-beam modules). The signal is pulled up to 3.3 V and reaches the Pico's GPn-1
# through a 1k resistor.

# The Pico's pins (top view, USB at the top): 1-20 down the left, 21-40 up the right
PICO_PINS = {
    1: "LANE1", 2: "LANE2", 3: "GND", 4: "LANE3", 5: "LANE4", 6: "LANE5", 7: "LANE6", 8: "GND",
    9: "LANE7", 10: "LANE8", 13: "GND", 18: "GND",
    21: "SDA", 22: "SCL", 23: "GND", 24: "GATE_IN", 25: "RESET_IN", 26: "LED_R_IN", 27: "LED_G_IN",
    28: "GND", 29: "LED_B_IN", 31: "AUX1", 32: "AUX2", 33: "GND", 34: "AUX3", 36: "+3V3", 38: "GND",
    39: "VSYS", 40: "VBUS",
}

PARTS = []


def part(ref, lib, name, value, x, y, rot=0, nets=None, desc="", mpn="", lcsc=""):
    PARTS.append(dict(ref=ref, lib=lib, name=name, value=value, x=x, y=y, rot=rot, nets=nets or {}, desc=desc,
                      mpn=mpn, lcsc=lcsc))


# Pico: left edge 52 mm in, 1.5 mm below the top edge (a notch in the edge clears the USB plug)
PICO_X, PICO_Y = 52.0, 1.5
part("U1", None, "RaspberryPi_Pico", "Raspberry Pi Pico", PICO_X, PICO_Y,
     nets=PICO_PINS, desc="Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), on 2x20 pin headers or soldered flat",
     mpn="SC0915")

ROW0, ROW_PITCH = 8.0, 9.2
for i in range(LANES):
    n = i + 1
    y = ROW0 + i * ROW_PITCH
    part(f"J{n}", "Connector_JST", "JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical", f"LANE {n}", 3.5, y,
         nets={1: f"LED{n}", 2: f"SENSE{n}", 3: "GND", 4: "VBUS"},
         desc="Lane sensor socket, JST XH 4-pin, 2.5 mm", mpn="JST B4B-XH-A", lcsc="C144395")
    part(f"R{n}", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "180", 18.5, y,
         nets={1: "VBUS", 2: f"LED{n}"}, desc="IR emitter current limit, 1/4 W (about 20 mA at 5 V)")
    part(f"R{10 + n}", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "10k", 29.5, y,
         nets={1: "+3V3", 2: f"SENSE{n}"}, desc="Sensor pull-up to 3.3 V, 1/4 W")
    part(f"R{20 + n}", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "1k", 40.5, y,
         nets={1: f"SENSE{n}", 2: f"LANE{n}"}, desc="Series protection for the Pico input, 1/4 W")

# Right-hand side: start gate, reset, status light, displays, spare pins
RX = 80.0
part("J9", "Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", "GATE", 101.0, 8.0,
     nets={1: "GATE", 2: "GND"}, desc="Start gate switch socket, JST XH 2-pin", mpn="JST B2B-XH-A", lcsc="C158012")
part("J10", "Connector_JST", "JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical", "RESET", 101.0, 18.0,
     nets={1: "RESET", 2: "GND"}, desc="Remote reset button socket, JST XH 2-pin", mpn="JST B2B-XH-A", lcsc="C158012")
part("R31", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "10k", RX, 11.0,
     nets={1: "+3V3", 2: "GATE"}, desc="Gate pull-up to 3.3 V, 1/4 W")
part("R32", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "1k", RX, 15.0,
     nets={1: "GATE", 2: "GATE_IN"}, desc="Series protection, 1/4 W")
part("R33", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "10k", RX, 19.0,
     nets={1: "+3V3", 2: "RESET"}, desc="Reset pull-up to 3.3 V, 1/4 W")
part("R34", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "1k", RX, 23.0,
     nets={1: "RESET", 2: "RESET_IN"}, desc="Series protection, 1/4 W")
part("SW1", "Button_Switch_THT", "SW_PUSH_6mm", "RESET", 96.0, 25.5,
     nets={1: "RESET", 2: "GND"}, desc="Reset button, 6x6 mm tactile, through-hole (4 legs)", mpn="any 6x6 mm THT, e.g. Omron B3F-1000")

part("J11", "Connector_PinHeader_2.54mm", "PinHeader_1x04_P2.54mm_Vertical", "DISPLAYS", 107.0, 37.0,
     nets={1: "GND", 2: "+3V3", 3: "SCL", 4: "SDA"},
     desc="I2C header for lane displays (HT16K33 4-digit backpacks, address 0x70 + lane - 1), 1x4 2.54 mm")
part("R35", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "4.7k", RX, 33.0,
     nets={1: "+3V3", 2: "SCL"}, desc="I2C pull-up, 1/4 W")
part("R36", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "4.7k", RX, 37.0,
     nets={1: "+3V3", 2: "SDA"}, desc="I2C pull-up, 1/4 W")

part("J12", "Connector_PinHeader_2.54mm", "PinHeader_1x06_P2.54mm_Vertical", "AUX", 107.0, 51.0,
     nets={1: "GND", 2: "+3V3", 3: "VBUS", 4: "AUX1", 5: "AUX2", 6: "AUX3"},
     desc="Spare pins: GP26, GP27, GP28 (buzzer, start solenoid driver, ...), 1x6 2.54 mm")

part("D1", "LED_THT", "LED_D5.0mm-4_RGB_Wide_Pins", "RGB", 84.0, 64.0,
     nets={1: "LED_R", 2: "GND", 3: "LED_G", 4: "LED_B"},
     desc="Status light, 5 mm RGB LED, common cathode, pins R-K-G-B (cathode longest)")
part("R37", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "220", RX, 47.0,
     nets={1: "LED_R_IN", 2: "LED_R"}, desc="Status light, red, 1/4 W")
part("R38", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "220", RX, 51.0,
     nets={1: "LED_G_IN", 2: "LED_G"}, desc="Status light, green, 1/4 W")
part("R39", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal", "220", RX, 55.0,
     nets={1: "LED_B_IN", 2: "LED_B"}, desc="Status light, blue, 1/4 W")

part("C1", "Capacitor_THT", "CP_Radial_D6.3mm_P2.50mm", "100uF 10V", 79.0, 4.8,
     nets={1: "VBUS", 2: "GND"}, desc="5 V supply bulk, electrolytic, 6.3 mm, 2.5 mm pitch")
part("C2", "Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "100nF", 87.0, 3.0,
     nets={1: "+3V3", 2: "GND"}, desc="3.3 V decoupling, ceramic, 5 mm pitch")
part("C3", "Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm", "100nF", 3.5, 78.5,
     nets={1: "VBUS", 2: "GND"}, desc="5 V decoupling, ceramic, 5 mm pitch")

for i, (x, y) in enumerate([(21.0, 78.5), (60.0, 78.5), (107.5, 78.5), (107.5, 30.5)]):
    part(f"H{i + 1}", "MountingHole", "MountingHole_3.2mm_M3", "M3", x, y, desc="M3 mounting hole")


# ------------------------------------------------------------------------------------------------ Footprints
def pico_footprint():
    """The Raspberry Pi Pico from its datasheet: 21 x 51 mm, 2x20 pins on 2.54 mm, rows 17.78 mm apart"""
    fp = pcbnew.FOOTPRINT(None)
    fp.SetFPID(pcbnew.LIB_ID("pinewood-time", "RaspberryPi_Pico"))
    fp.SetAttributes(pcbnew.FP_THROUGH_HOLE)
    for k in range(1, 41):
        pad = pcbnew.PAD(fp)
        pad.SetNumber(str(k))
        pad.SetAttribute(pcbnew.PAD_ATTRIB_PTH)
        pad.SetShape(pcbnew.PAD_SHAPE_RECT if k == 1 else pcbnew.PAD_SHAPE_OVAL)
        pad.SetSize(pcbnew.VECTOR2I(pcbnew.FromMM(1.7), pcbnew.FromMM(1.7)))
        pad.SetDrillSize(pcbnew.VECTOR2I(pcbnew.FromMM(1.0), pcbnew.FromMM(1.0)))
        pad.SetLayerSet(pad.PTHMask())
        if k <= 20:
            x, y = 1.61, 1.37 + (k - 1) * 2.54
        else:
            x, y = 1.61 + 17.78, 1.37 + (40 - k) * 2.54
        pos = pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))
        pad.SetPos0(pos)  # relative to the footprint (KiCad 7)
        pad.SetPosition(pos)
        fp.Add(pad)

    def line(layer, x1, y1, x2, y2, width=0.12):
        s = pcbnew.FP_SHAPE(fp)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(layer)
        s.SetStart0(pcbnew.VECTOR2I(pcbnew.FromMM(x1), pcbnew.FromMM(y1)))
        s.SetEnd0(pcbnew.VECTOR2I(pcbnew.FromMM(x2), pcbnew.FromMM(y2)))
        s.SetWidth(pcbnew.FromMM(width))
        fp.Add(s)

    # Outline (silkscreen) and courtyard
    for layer, inset, wid in [(pcbnew.F_CrtYd, -0.25, 0.05), (pcbnew.F_Fab, 0.0, 0.1)]:
        x0, y0, x1, y1 = inset, inset, 21.0 - inset, 51.0 - inset
        line(layer, x0, y0, x1, y0, wid)
        line(layer, x1, y0, x1, y1, wid)
        line(layer, x1, y1, x0, y1, wid)
        line(layer, x0, y1, x0, y0, wid)
    # Silkscreen outline, left open at the top where the board's notch clears the USB plug
    line(pcbnew.F_SilkS, 0.0, 0.6, 0.0, 51.0, 0.15)
    line(pcbnew.F_SilkS, 0.0, 51.0, 21.0, 51.0, 0.15)
    line(pcbnew.F_SilkS, 21.0, 51.0, 21.0, 0.6, 0.15)
    for text, x, y, size in [("USB", 10.5, 6.8, 1.0), ("Raspberry Pi", 10.5, 23.0, 1.2), ("Pico", 10.5, 25.0, 1.2),
                             ("1", -1.5, 1.37, 0.8), ("GP7", 4.4, 1.37 + 9 * 2.54, 0.8),
                             ("GP16", 16.2, 1.37 + 19 * 2.54, 0.8), ("3V3", 16.6, 1.37 + 4 * 2.54, 0.8)]:
        t = pcbnew.FP_TEXT(fp)
        t.SetText(text)
        t.SetLayer(pcbnew.F_SilkS)
        t.SetPos0(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
        t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size)))
        t.SetTextThickness(pcbnew.FromMM(size * 0.15))
        fp.Add(t)
    return fp


def load(p):
    if p["lib"] is None:
        fp = pico_footprint()
    else:
        fp = pcbnew.FootprintLoad(os.path.join(FP, p["lib"] + ".pretty"), p["name"])
        if fp is None:
            raise SystemExit(f"Footprint not found: {p['lib']}:{p['name']}")
        fp.SetFPID(pcbnew.LIB_ID(p["lib"], p["name"]))
    fp.SetReference(p["ref"])
    fp.SetValue(p["value"])
    ref = fp.Reference()
    if p["lib"] is None:
        # The Pico: its reference in the middle, no value text
        ref.SetPos0(pcbnew.VECTOR2I(pcbnew.FromMM(10.5), pcbnew.FromMM(28.0)))
        ref.SetPosition(ref.GetPos0())
        fp.Value().SetVisible(False)
    elif p["ref"].startswith("H"):
        ref.SetVisible(False)
        fp.Value().SetVisible(False)
    elif p["name"].startswith("CP_Radial"):
        # Electrolytic: beside the can, which fills its outline
        ref.SetPos0(pcbnew.VECTOR2I(pcbnew.FromMM(5.4), 0))
        ref.SetPosition(ref.GetPos0())
        ref.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8), pcbnew.FromMM(0.8)))
        ref.SetTextThickness(pcbnew.FromMM(0.12))
    elif p["ref"][0] in "RC":
        # Resistors and capacitors: the reference inside the part's outline, clear of its neighbours
        pads = [pad.GetPos0() for pad in fp.Pads()]
        cx = sum(v.x for v in pads) // len(pads)
        cy = sum(v.y for v in pads) // len(pads)
        ref.SetPos0(pcbnew.VECTOR2I(cx, cy))
        ref.SetPosition(ref.GetPos0())
        ref.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8), pcbnew.FromMM(0.8)))
        ref.SetTextThickness(pcbnew.FromMM(0.12))
    return fp


# ------------------------------------------------------------------------------------------------ Board
def outline(board):
    """The board edge, with a notch at the top that clears the Pico's USB plug"""
    notch_l, notch_r, notch_d = PICO_X + 5.0, PICO_X + 16.0, 6.0
    pts = [(0, 0), (notch_l, 0), (notch_l, notch_d), (notch_r, notch_d), (notch_r, 0), (W, 0), (W, H), (0, H), (0, 0)]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetStart(mm(x1, y1))
        seg.SetEnd(mm(x2, y2))
        seg.SetWidth(pcbnew.FromMM(0.1))
        board.Add(seg)


def text(board, s, x, y, size=1.0, layer=pcbnew.F_SilkS, bold=False, justify_left=False, justify_right=False):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetLayer(layer)
    t.SetPosition(mm(x, y))
    t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size)))
    t.SetTextThickness(pcbnew.FromMM(size * (0.2 if bold else 0.15)))
    if justify_left:
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
    if justify_right:
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_RIGHT)
    if layer == pcbnew.B_SilkS:
        t.SetMirrored(True)
    board.Add(t)


def write_project(path):
    """Design rules (JLCPCB-friendly) and net classes: power nets get wider tracks"""
    pro = {
        "board": {
            "design_settings": {
                "rule_severities": {"lib_footprint_issues": "ignore", "lib_footprint_mismatch": "ignore"},
                "defaults": {"board_outline_line_width": 0.1, "copper_line_width": 0.2},
                "rules": {
                    "min_clearance": 0.2, "min_track_width": 0.2, "min_via_diameter": 0.6, "min_through_hole_diameter": 0.3,
                    "min_hole_to_hole": 0.25, "min_copper_edge_clearance": 0.5, "min_hole_clearance": 0.25,
                    "min_silk_clearance": 0.0,
                },
                "track_widths": [0.0, 0.3, 0.6, 0.8],
                "via_dimensions": [{"diameter": 0.0, "drill": 0.0}, {"diameter": 0.8, "drill": 0.4}],
            }
        },
        "meta": {"filename": "pinewood-time.kicad_pro", "version": 1},
        "net_settings": {
            "classes": [
                {"name": "Default", "clearance": 0.25, "track_width": 0.3, "via_diameter": 0.8, "via_drill": 0.4,
                 "diff_pair_gap": 0.25, "diff_pair_width": 0.2, "microvia_diameter": 0.3, "microvia_drill": 0.1,
                 "line_style": 0, "wire_width": 6, "bus_width": 12, "pcb_color": "rgba(0, 0, 0, 0.000)",
                 "schematic_color": "rgba(0, 0, 0, 0.000)"},
                {"name": "Power", "clearance": 0.3, "track_width": 0.6, "via_diameter": 1.0, "via_drill": 0.5,
                 "diff_pair_gap": 0.25, "diff_pair_width": 0.2, "microvia_diameter": 0.3, "microvia_drill": 0.1,
                 "line_style": 0, "wire_width": 6, "bus_width": 12, "pcb_color": "rgba(0, 0, 0, 0.000)",
                 "schematic_color": "rgba(0, 0, 0, 0.000)"},
            ],
            "meta": {"version": 3},
            "netclass_patterns": [{"netclass": "Power", "pattern": p} for p in ("VBUS", "+3V3", "GND", "VSYS")],
        },
    }
    with open(path, "w") as f:
        json.dump(pro, f, indent=2)


def write_libraries():
    """A project library holding the Pico footprint, and a library table pointing at it and KiCad's own"""
    lib = os.path.join(HERE, "pinewood-time.pretty")
    os.makedirs(lib, exist_ok=True)
    pcbnew.FootprintSave(lib, pico_footprint())
    used = sorted({p["lib"] for p in PARTS if p["lib"]})
    rows = ['  (lib (name "pinewood-time")(type "KiCad")(uri "${KIPRJMOD}/pinewood-time.pretty")(options "")(descr "This project\'s footprints"))']
    rows += [f'  (lib (name "{name}")(type "KiCad")(uri "${{KICAD7_FOOTPRINT_DIR}}/{name}.pretty")(options "")(descr ""))'
             for name in used]
    with open(os.path.join(HERE, "fp-lib-table"), "w") as f:
        f.write("(fp_lib_table\n  (version 7)\n" + "\n".join(rows) + "\n)\n")


def place():
    write_libraries()
    board = pcbnew.BOARD()
    board.SetCopperLayerCount(2)
    outline(board)
    nets = {}

    def net(name):
        if name not in nets:
            n = pcbnew.NETINFO_ITEM(board, name)
            board.Add(n)
            nets[name] = n
        return nets[name]

    for p in PARTS:
        fp = load(p)
        board.Add(fp)
        fp.SetPosition(mm(p["x"], p["y"]))
        if p["rot"]:
            fp.SetOrientationDegrees(p["rot"])
        for pad in fp.Pads():
            num = pad.GetNumber()
            key = int(num) if num.isdigit() else num
            if key in p["nets"]:
                pad.SetNet(net(p["nets"][key]))
    write_project(PROJECT)
    pcbnew.SaveBoard(BOARD, board)
    print(f"placed {len(PARTS)} parts:", BOARD)


def ground_pour(board, layer):
    zone = pcbnew.ZONE(board)
    zone.SetLayer(layer)
    zone.SetNetCode(board.GetNetcodeFromNetname("GND"))
    zone.SetIsRuleArea(False)
    zone.SetLocalClearance(pcbnew.FromMM(0.3))
    zone.SetMinThickness(pcbnew.FromMM(0.25))
    zone.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    zone.SetThermalReliefGap(pcbnew.FromMM(0.4))
    zone.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.5))
    poly = zone.Outline()
    poly.NewOutline()
    for x, y in [(0.3, 0.3), (W - 0.3, 0.3), (W - 0.3, H - 0.3), (0.3, H - 0.3)]:
        poly.Append(mm(x, y))
    board.Add(zone)
    return zone


def labels(board):
    # The labelled sockets and the LED say what they are in words instead of J1, J9...
    named = {f"J{i + 1}" for i in range(LANES)} | {"J9", "J10", "D1"}
    for fp in board.GetFootprints():
        if fp.GetReference() in named:
            fp.Reference().SetVisible(False)
    # Each lane socket: its number, and (once, at the top) what each pin is
    for i in range(LANES):
        y = ROW0 + i * ROW_PITCH
        text(board, f"LANE {i + 1}", 1.0, y - 4.4, 1.0, bold=True, justify_left=True)
    text(board, "LED+ SIG GND 5V", 8.6, ROW0 - 4.4, 0.8, justify_left=True)
    text(board, "IR LED      PULL-UP   SERIES", 30.0, ROW0 - 4.4, 0.8)
    # Gate and reset sockets
    text(board, "GATE  SIG GND", 104.0, 4.4, 0.8)
    text(board, "RESET  SW GND", 104.0, 14.4, 0.8)
    # The two headers run downwards: name each pin beside it
    for k, name in enumerate(["GND", "3V3", "SCL", "SDA"]):
        text(board, name, 105.2, 37.0 + k * 2.54, 0.8, justify_right=True)
    text(board, "DISPLAYS", 105.2, 34.6, 0.8, justify_right=True)
    for k, name in enumerate(["GND", "3V3", "5V", "GP26", "GP27", "GP28"]):
        text(board, name, 105.2, 51.0 + k * 2.54, 0.8, justify_right=True)
    text(board, "AUX", 105.2, 48.6, 0.8, justify_right=True)
    # Status LED: its leg order above, its name below
    text(board, "R  K  G  B", 87.1, 60.6, 0.8)
    text(board, "STATUS", 87.1, 68.6, 0.8)
    # Title, and a note on the back
    text(board, "pinewood-time", 36.5, 77.4, 2.0, bold=True)
    text(board, f"Pico race timer {VERSION}", 36.5, 80.3, 1.0)
    for k, line in enumerate([f"pinewood-time {VERSION}", "github.com/no5tyle/pinewood-time",
                              "lanes GP0-7  gate GP18  reset GP19", "LED GP20-22  I2C GP16/17"]):
        text(board, line, 78.0, 70.4 + k * 1.6, 0.9, layer=pcbnew.B_SilkS)


def starved_pads(board):
    import re
    import tempfile
    pcbnew.SaveBoard(BOARD, board)
    with tempfile.TemporaryDirectory() as tmp:
        report = os.path.join(tmp, "drc.rpt")
        pcbnew.WriteDRCReport(pcbnew.LoadBoard(BOARD), report, pcbnew.EDA_UNITS_MILLIMETRES, False)
        text = open(report).read()
    found = set()
    for block in text.split("[starved_thermal]")[1:]:
        m = re.search(r"pad (\w+) \[GND\] of (\w+)", block.split("\n[")[0])
        if m:
            found.add((m.group(2), m.group(1)))
    return found


def finish():
    board = pcbnew.LoadBoard(BOARD)
    ground_pour(board, pcbnew.F_Cu)
    ground_pour(board, pcbnew.B_Cu)
    labels(board)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    # Where tracks crowd a ground pad there's only room for one thermal spoke: join those pads to the pour
    # solidly instead. KiCad's own check finds them.
    for _ in range(3):
        starved = starved_pads(board)
        if not starved:
            break
        for fp in board.GetFootprints():
            for pad in fp.Pads():
                if (fp.GetReference(), pad.GetNumber()) in starved:
                    pad.SetZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
        pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(BOARD, board)
    print("poured and labelled:", BOARD)


if __name__ == "__main__":
    step = sys.argv[1] if len(sys.argv) > 1 else "place"
    {"place": place, "finish": finish}[step]()
