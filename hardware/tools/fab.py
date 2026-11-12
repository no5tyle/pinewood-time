#!/usr/bin/python3
"""
Manufacturing files: /usr/bin/python3 hardware/tools/fab.py

  hardware/fab/pinewood-time-gerbers.zip   Gerbers + Excellon drill, as JLCPCB/PCBWay/OSH Park take them
  hardware/fab/bom.csv                     parts for one board, grouped (also in hardware/fab/bom.md)
"""
import csv
import glob
import os
import shutil
import subprocess
import sys
import zipfile
from collections import OrderedDict

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HERE)
os.environ.setdefault("KICAD7_FOOTPRINT_DIR", "/usr/share/kicad/footprints")
import generate_board  # noqa: E402  (the parts list lives there)

BOARD = os.path.join(HERE, "pinewood-time.kicad_pcb")
FAB = os.path.join(HERE, "fab")
LAYERS = ["F.Cu", "B.Cu", "F.Paste", "B.Paste", "F.Silkscreen", "B.Silkscreen", "F.Mask", "B.Mask", "Edge.Cuts"]


def gerbers():
    tmp = os.path.join(HERE, "build", "gerbers")
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    subprocess.run(["kicad-cli", "pcb", "export", "gerbers", "--layers", ",".join(LAYERS), "--subtract-soldermask",
                    "-o", tmp + "/", BOARD], check=True, capture_output=True)
    subprocess.run(["kicad-cli", "pcb", "export", "drill", "--format", "excellon", "--excellon-separate-th",
                    "--generate-map", "--map-format", "pdf", "-o", tmp + "/", BOARD], check=True, capture_output=True)
    files = sorted(f for f in glob.glob(tmp + "/*") if not f.endswith(".pdf"))
    out = os.path.join(FAB, "pinewood-time-gerbers.zip")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for f in files:
            z.write(f, os.path.basename(f))
    for f in glob.glob(tmp + "/*.pdf"):
        shutil.copy(f, os.path.join(FAB, os.path.basename(f)))
    return out, [os.path.basename(f) for f in files]


SHORT = {"Connector_JST": None, "Connector_PinHeader_2.54mm": None}


def bom():
    """One line per distinct part (value + footprint), with every use of it listed"""
    groups = OrderedDict()
    for p in generate_board.PARTS:
        if p["lib"] == "MountingHole":
            continue
        g = groups.setdefault((p["value"] if p["lib"] not in SHORT else "", p["name"]),
                              dict(refs=[], p=p, uses=[]))
        g["refs"].append(p["ref"])
        use = p["desc"]
        if use not in g["uses"]:
            g["uses"].append(use)
    rows = []
    for g in groups.values():
        p = g["p"]
        value = p["value"]
        if p["lib"] == "Connector_JST":
            value = p["name"].split("_")[2]  # B4B-XH-A
        elif p["lib"] == "Connector_PinHeader_2.54mm":
            value = "1x%s header" % p["name"].split("_")[1][2:]
        rows.append([len(g["refs"]), " ".join(g["refs"]), value, "; ".join(g["uses"]), p["name"], p["mpn"],
                     p["lcsc"]])
    head = ["Qty", "References", "Value", "Description", "Footprint", "Part number", "LCSC"]
    with open(os.path.join(FAB, "bom.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(head)
        w.writerows(rows)
    with open(os.path.join(FAB, "bom.md"), "w") as f:
        f.write("# pinewood-time board: parts for one board\n\n")
        f.write("All through-hole. Resistors are 1/4 W, 5% or better, 0207 size (6.3 mm body, 7.62 mm hole spacing).\n\n")
        f.write("| " + " | ".join(head[:4] + head[5:]) + " |\n|" + "---|" * (len(head) - 1) + "\n")
        for r in rows:
            f.write("| " + " | ".join(str(c).replace("|", "/") for c in r[:4] + r[5:]) + " |\n")
    return rows


def main():
    os.makedirs(FAB, exist_ok=True)
    out, files = gerbers()
    rows = bom()
    print(f"{out}: {', '.join(files)}")
    print(f"bom: {sum(r[0] for r in rows)} parts in {len(rows)} lines")


if __name__ == "__main__":
    main()
