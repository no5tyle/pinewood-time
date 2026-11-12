#!/usr/bin/python3
"""
Renders pictures of the board, front and back, as it'll look made: /usr/bin/python3 hardware/tools/render.py

KiCad 7 can't render from the command line, so this exports each layer on its own (black and white SVG,
all the same size) and stacks them in colour: solder mask, copper under it, gold where pads are exposed,
white silkscreen and the holes. The stacking is done in a browser (tools/render.js, Playwright).
"""
import os
import subprocess
import sys

os.environ.setdefault("KICAD7_FOOTPRINT_DIR", "/usr/share/kicad/footprints")
import pcbnew  # noqa: E402

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOARD = os.path.join(HERE, "pinewood-time.kicad_pcb")
OUT = os.path.join(HERE, "build", "render")


def prepare():
    """A copy of the board with its shape filled in on Dwgs.User and every hole on Cmts.User"""
    board = pcbnew.LoadBoard(BOARD)
    outline = pcbnew.SHAPE_POLY_SET()
    board.GetBoardPolygonOutlines(outline)
    shape = pcbnew.PCB_SHAPE(board)
    shape.SetShape(pcbnew.SHAPE_T_POLY)
    shape.SetPolyShape(outline)
    shape.SetFilled(True)
    shape.SetWidth(0)
    shape.SetLayer(pcbnew.Dwgs_User)
    board.Add(shape)
    holes = [(p.GetPosition(), p.GetDrillSize().x) for fp in board.GetFootprints() for p in fp.Pads()
             if p.GetDrillSize().x > 0]
    holes += [(t.GetPosition(), t.GetDrillValue()) for t in board.GetTracks() if isinstance(t, pcbnew.PCB_VIA)]
    for pos, d in holes:
        c = pcbnew.PCB_SHAPE(board)
        c.SetShape(pcbnew.SHAPE_T_CIRCLE)
        c.SetCenter(pos)
        c.SetEnd(pcbnew.VECTOR2I(pos.x + d // 2, pos.y))
        c.SetFilled(True)
        c.SetWidth(0)
        c.SetLayer(pcbnew.Cmts_User)
        board.Add(c)
    path = os.path.join(OUT, "render.kicad_pcb")
    pcbnew.SaveBoard(path, board)
    return path


def export(path, layer, name, mirror):
    out = os.path.join(OUT, name + ".svg")
    # Edge.Cuts goes in every export so they all come out exactly the same size
    cmd = ["kicad-cli", "pcb", "export", "svg", "--layers", f"{layer},Edge.Cuts", "--black-and-white",
           "--exclude-drawing-sheet", "--page-size-mode", "2", "--drill-shape-opt", "0", "-o", out, path]
    if mirror:
        cmd.insert(4, "--mirror")
    subprocess.run(cmd, check=True, capture_output=True)
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    path = prepare()
    for side, mirror in (("front", False), ("back", True)):
        p = "F" if side == "front" else "B"
        layers = [
            (export(path, "Dwgs.User", f"{side}-board", mirror), "#155c34"),  # mask over bare board
            (export(path, f"{p}.Cu", f"{side}-copper", mirror), "#1f7a45"),  # copper shows through the mask
            (export(path, f"{p}.Mask", f"{side}-pads", mirror), "#d6b25e"),  # mask openings: gold pads
            (export(path, f"{p}.Silkscreen", f"{side}-silk", mirror), "#f4f4ef"),
            (export(path, "Cmts.User", f"{side}-holes", mirror), "#1b1b1b"),
        ]
        png = os.path.join(HERE, "renders", f"pcb-{side}.png")
        os.makedirs(os.path.dirname(png), exist_ok=True)
        args = ["node", os.path.join(HERE, "tools", "render.js"), png] + [f"{s}={c}" for s, c in layers]
        subprocess.run(args, check=True, cwd=os.environ.get("PLAYWRIGHT_CWD", HERE))
    print("renders in", os.path.join(HERE, "renders"))


if __name__ == "__main__":
    sys.exit(main())
