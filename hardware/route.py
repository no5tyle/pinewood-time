#!/usr/bin/python3
"""
A small grid router for the pinewood-time board: /usr/bin/python3 hardware/route.py

Routes every net except GND (which comes from copper pours on both layers) as tracks on the two copper
layers, with vias where a track changes layer. It searches a 0.2 mm grid with A*, keeping every track the
design rules' clearance away from other nets' pads, tracks and vias and from the board edge, then writes
the tracks into the board. KiCad's design rule check (hardware/drc.py) checks the result exactly.

Power nets (VBUS, +3V3) get 0.6 mm tracks, signals 0.3 mm. A multi-pin net is grown as a tree: each pin in
turn joins the nearest part of what's already routed.
"""
import heapq
import math
import os

import numpy as np
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
BOARD = os.path.join(HERE, "pinewood-time.kicad_pcb")

RES = 0.2  # grid, mm
CLEARANCE = 0.3  # mm, a little over the design rules' 0.25 so the grid's rounding can't break them
EDGE = 0.6  # copper to board edge, mm
VIA_D, VIA_DRILL = 0.8, 0.4
VIA_COST = 25.0
POWER = {"VBUS": 0.6, "+3V3": 0.6, "VSYS": 0.6}
SIGNAL_W = 0.3
LAYERS = [pcbnew.F_Cu, pcbnew.B_Cu]
# Gentle preferences that keep the layers from fighting: front mostly horizontal, back mostly vertical
DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)]


def to_mm(v):
    return pcbnew.ToMM(v)


class Grid:
    def __init__(self, board):
        bb = board.GetBoardEdgesBoundingBox()
        self.x0, self.y0 = to_mm(bb.GetX()), to_mm(bb.GetY())
        self.w = int(math.ceil(to_mm(bb.GetWidth()) / RES)) + 1
        self.h = int(math.ceil(to_mm(bb.GetHeight()) / RES)) + 1
        self.xs = self.x0 + np.arange(self.w) * RES
        self.ys = self.y0 + np.arange(self.h) * RES
        # Inside the board, at least EDGE from any edge (the outline is rectilinear: test against its segments)
        self.edges = []
        for d in board.GetDrawings():
            if d.GetLayer() == pcbnew.Edge_Cuts and d.GetShape() == pcbnew.SHAPE_T_SEGMENT:
                s, e = d.GetStart(), d.GetEnd()
                self.edges.append((to_mm(s.x), to_mm(s.y), to_mm(e.x), to_mm(e.y)))
        self.inside = self._inside_mask(board)

    def _inside_mask(self, board):
        outline = pcbnew.SHAPE_POLY_SET()
        board.GetBoardPolygonOutlines(outline)
        X, Y = np.meshgrid(self.xs, self.ys)
        # Point-in-polygon on the outline's vertices
        ol = outline.Outline(0)
        pts = [(to_mm(ol.CPoint(k).x), to_mm(ol.CPoint(k).y)) for k in range(ol.PointCount())]
        inside = _point_in_poly(X, Y, pts)
        # and clear of each edge by EDGE (plus the track's half width, added per net)
        self.edge_dist = np.full((self.h, self.w), 1e9)
        for x1, y1, x2, y2 in self.edges:
            self.edge_dist = np.minimum(self.edge_dist, _seg_dist(X, Y, x1, y1, x2, y2))
        return inside

    def cell(self, x, y):
        return int(round((x - self.x0) / RES)), int(round((y - self.y0) / RES))

    def window(self, x1, y1, x2, y2):
        i1 = max(0, int(math.floor((x1 - self.x0) / RES)))
        i2 = min(self.w, int(math.ceil((x2 - self.x0) / RES)) + 1)
        j1 = max(0, int(math.floor((y1 - self.y0) / RES)))
        j2 = min(self.h, int(math.ceil((y2 - self.y0) / RES)) + 1)
        return i1, i2, j1, j2


def _point_in_poly(X, Y, pts):
    inside = np.zeros(X.shape, bool)
    n = len(pts)
    for k in range(n):
        x1, y1 = pts[k]
        x2, y2 = pts[(k + 1) % n]
        cond = ((y1 > Y) != (y2 > Y))
        with np.errstate(divide="ignore", invalid="ignore"):
            xint = (x2 - x1) * (Y - y1) / (y2 - y1 + 1e-12) + x1
        inside ^= cond & (X < xint)
    return inside


def _seg_dist(X, Y, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    L2 = dx * dx + dy * dy
    if L2 == 0:
        return np.hypot(X - x1, Y - y1)
    t = np.clip(((X - x1) * dx + (Y - y1) * dy) / L2, 0, 1)
    return np.hypot(X - (x1 + t * dx), Y - (y1 + t * dy))


class Copper:
    """A piece of copper on some layers: a pad, track or via, as a capsule (segment + radius) or a rectangle"""

    def __init__(self, net, layers, kind, **g):
        self.net, self.layers, self.kind, self.g = net, layers, kind, g

    def block(self, grid, mask, layer_index, inflate):
        g = self.g
        if self.kind == "rect":
            cx, cy, hw, hh = g["cx"], g["cy"], g["hw"] + inflate, g["hh"] + inflate
            i1, i2, j1, j2 = grid.window(cx - hw, cy - hh, cx + hw, cy + hh)
            if i1 >= i2 or j1 >= j2:
                return
            X, Y = np.meshgrid(grid.xs[i1:i2], grid.ys[j1:j2])
            m = (np.abs(X - cx) <= hw) & (np.abs(Y - cy) <= hh)
        else:  # capsule
            x1, y1, x2, y2, r = g["x1"], g["y1"], g["x2"], g["y2"], g["r"] + inflate
            i1, i2, j1, j2 = grid.window(min(x1, x2) - r, min(y1, y2) - r, max(x1, x2) + r, max(y1, y2) + r)
            if i1 >= i2 or j1 >= j2:
                return
            X, Y = np.meshgrid(grid.xs[i1:i2], grid.ys[j1:j2])
            m = _seg_dist(X, Y, x1, y1, x2, y2) <= r
        mask[layer_index, j1:j2, i1:i2] |= m


def pad_copper(pad):
    net = pad.GetNetname()
    pos = pad.GetPosition()
    cx, cy = to_mm(pos.x), to_mm(pos.y)
    sx, sy = to_mm(pad.GetSize().x), to_mm(pad.GetSize().y)
    rot = pad.GetOrientationDegrees() % 180
    if rot in (90.0,):
        sx, sy = sy, sx
    shape = pad.GetShape()
    if shape in (pcbnew.PAD_SHAPE_CIRCLE,) or (shape == pcbnew.PAD_SHAPE_OVAL and abs(sx - sy) < 1e-6):
        return Copper(net, (0, 1), "capsule", x1=cx, y1=cy, x2=cx, y2=cy, r=sx / 2)
    if shape == pcbnew.PAD_SHAPE_OVAL:
        if sx > sy:
            d = (sx - sy) / 2
            return Copper(net, (0, 1), "capsule", x1=cx - d, y1=cy, x2=cx + d, y2=cy, r=sy / 2)
        d = (sy - sx) / 2
        return Copper(net, (0, 1), "capsule", x1=cx, y1=cy - d, x2=cx, y2=cy + d, r=sx / 2)
    return Copper(net, (0, 1), "rect", cx=cx, cy=cy, hw=sx / 2, hh=sy / 2)


def route():
    board = pcbnew.LoadBoard(BOARD)
    for t in list(board.GetTracks()):
        board.Remove(t)
    grid = Grid(board)

    copper = []  # everything placed so far: pads, then tracks and vias as they're routed
    pads_by_net = {}
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            c = pad_copper(pad)
            if pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                # mounting hole: blocks both layers
                c.net = "__hole__"
                c.g["r"] = to_mm(pad.GetDrillSize().x) / 2
            copper.append(c)
            if pad.GetNetname():
                pads_by_net.setdefault(pad.GetNetname(), []).append((pad, c))

    nets = [n for n, ps in pads_by_net.items() if n != "GND" and len(ps) >= 2]
    # Power first (widest, most pins), then signals from shortest to longest
    def span(n):
        ps = [(to_mm(p.GetPosition().x), to_mm(p.GetPosition().y)) for p, _ in pads_by_net[n]]
        return max(math.dist(a, b) for a in ps for b in ps)
    nets.sort(key=lambda n: (0 if n in POWER else 1, span(n) if n not in POWER else -len(pads_by_net[n])))

    failed = []
    for name in nets:
        w = POWER.get(name, SIGNAL_W)
        if not route_net(board, grid, copper, name, pads_by_net[name], w):
            failed.append(name)
    pcbnew.SaveBoard(BOARD, board)
    print(f"routed {len(nets) - len(failed)} of {len(nets)} nets" + (f"; failed: {failed}" if failed else ""))
    return failed


def blocked_for(grid, copper, net, w):
    """Cells a track of this net and width can't run along, per layer, and cells a via can't sit on"""
    mask = np.zeros((2, grid.h, grid.w), bool)
    for c in copper:
        if c.net == net:
            continue
        for li in c.layers:
            c.block(grid, mask, li, w / 2 + CLEARANCE)
    outside = ~grid.inside | (grid.edge_dist < EDGE + w / 2)
    mask[0] |= outside
    mask[1] |= outside
    vmask = np.zeros((2, grid.h, grid.w), bool)
    for c in copper:
        if c.net == net:
            continue
        for li in c.layers:
            c.block(grid, vmask, li, VIA_D / 2 + CLEARANCE)
    via = vmask[0] | vmask[1] | ~grid.inside | (grid.edge_dist < EDGE + VIA_D / 2)
    return mask, via


def pad_cells(grid, pad):
    """Grid cells inside a pad, where a track may start or end"""
    c = pad_copper(pad)
    m = np.zeros((2, grid.h, grid.w), bool)
    c.block(grid, m, 0, -0.05)
    cells = list(zip(*np.nonzero(m[0])))
    if not cells:
        i, j = grid.cell(to_mm(pad.GetPosition().x), to_mm(pad.GetPosition().y))
        cells = [(j, i)]
    return cells


def route_net(board, grid, copper, name, pads, w):
    mask, via_blocked = blocked_for(grid, copper, name, w)
    H, W = grid.h, grid.w
    blocked = [mask[0].ravel().tolist(), mask[1].ravel().tolist()]
    viab = via_blocked.ravel().tolist()
    net = board.FindNet(name)

    # The tree starts at the first pad; the rest join one at a time, nearest first
    tree = set()
    start = pads[0][0]
    for (j, i) in pad_cells(grid, start):
        for li in (0, 1):
            tree.add((li, j * W + i))
    remaining = [p for p, _ in pads[1:]]
    ok = True
    while remaining:
        def nearest(p):
            px, py = to_mm(p.GetPosition().x), to_mm(p.GetPosition().y)
            best = 1e9
            for (li, idx) in list(tree)[::7] or list(tree):
                j, i = divmod(idx, W)
                best = min(best, math.hypot(grid.xs[i] - px, grid.ys[j] - py))
            return best
        remaining.sort(key=nearest)
        target_pad = remaining.pop(0)
        targets = set()
        for (j, i) in pad_cells(grid, target_pad):
            for li in (0, 1):
                targets.add((li, j * W + i))
        path = astar(grid, blocked, viab, tree, targets)
        if not path:
            print(f"  could not route {name} to {target_pad.GetParent().GetReference()}.{target_pad.GetNumber()}")
            ok = False
            continue
        before = len(copper)
        add_path(board, grid, copper, net, name, w, path, target_pad, blocked, targets | tree)
        # Later branches may start anywhere on the copper just laid (not on the grid path: straightening
        # moved it), so the tree grows by the cells that copper really covers
        tree |= covered_cells(grid, copper[before:], w)
        # the pad itself becomes part of the tree too
        for node in targets:
            tree.add(node)
    return ok


def astar(grid, blocked, viab, sources, targets):
    W, H = grid.w, grid.h
    tx = [divmod(idx, W) for (_, idx) in targets]
    tj = sum(j for j, _ in tx) / len(tx)
    ti = sum(i for _, i in tx) / len(tx)

    def h(idx):
        j, i = divmod(idx, W)
        return math.hypot(j - tj, i - ti) * 0.98

    openq = []
    g = {}
    came = {}
    for node in sources:
        li, idx = node
        if blocked[li][idx] and node not in targets:
            # a pad's own cells are free for its net; anything else blocked can't be a start
            pass
        g[node] = 0.0
        heapq.heappush(openq, (h(idx), 0.0, node))
    seen = set()
    while openq:
        f, gc, node = heapq.heappop(openq)
        if node in seen:
            continue
        seen.add(node)
        if node in targets and node not in sources:
            path = [node]
            while path[-1] in came:
                path.append(came[path[-1]])
            return path[::-1]
        li, idx = node
        j, i = divmod(idx, W)
        for di, dj in DIRS:
            ni, nj = i + di, j + dj
            if ni < 0 or nj < 0 or ni >= W or nj >= H:
                continue
            nidx = nj * W + ni
            nn = (li, nidx)
            if blocked[li][nidx] and nn not in targets:
                continue
            if di and dj:
                # diagonal: both side cells must be free too, so the track doesn't clip a corner
                if blocked[li][j * W + ni] and (li, j * W + ni) not in targets:
                    continue
                if blocked[li][nj * W + i] and (li, nj * W + i) not in targets:
                    continue
                step = 1.414
            else:
                step = 1.0
            # front horizontal, back vertical: going against the grain costs a little more
            if (li == 0 and dj and not di) or (li == 1 and di and not dj):
                step *= 1.6
            ng = gc + step
            if ng < g.get(nn, 1e18):
                g[nn] = ng
                came[nn] = node
                heapq.heappush(openq, (ng + h(nidx), ng, nn))
        # change layer with a via
        other = (1 - li, idx)
        if not viab[idx] and not blocked[1 - li][idx]:
            ng = gc + VIA_COST
            if ng < g.get(other, 1e18):
                g[other] = ng
                came[other] = node
                heapq.heappush(openq, (ng + h(idx), ng, other))
    return None


def add_path(board, grid, copper, net, name, w, path, target_pad, blocked, own):
    W = grid.w

    def xy(idx):
        j, i = divmod(idx, W)
        return float(grid.xs[i]), float(grid.ys[j])

    # Split into runs on one layer; join each run's straight stretches into single tracks
    runs = []
    cur = [path[0]]
    for a, b in zip(path, path[1:]):
        if a[0] != b[0]:
            runs.append(cur)
            cur = [b]
            # via between a and b (same cell)
            x, y = xy(a[1])
            v = pcbnew.PCB_VIA(board)
            v.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
            v.SetWidth(pcbnew.FromMM(VIA_D))
            v.SetDrill(pcbnew.FromMM(VIA_DRILL))
            v.SetNet(net)
            board.Add(v)
            copper.append(Copper(name, (0, 1), "capsule", x1=x, y1=y, x2=x, y2=y, r=VIA_D / 2))
        else:
            cur.append(b)
    runs.append(cur)
    for run in runs:
        li = run[0][0]
        pts = [xy(n[1]) for n in run]
        corners = smooth(grid, blocked[li], {idx for (l, idx) in own if l == li}, [n[1] for n in run])
        corners = [xy(idx) for idx in corners]
        for (x1, y1), (x2, y2) in zip(corners, corners[1:]):
            add_track(board, copper, net, name, LAYERS[li], li, w, x1, y1, x2, y2)
    # Finish at the pads' centres, so KiCad sees the track reach them
    start_node, end_node = path[0], path[-1]
    tx, ty = to_mm(target_pad.GetPosition().x), to_mm(target_pad.GetPosition().y)
    ex, ey = xy(end_node[1])
    if math.hypot(ex - tx, ey - ty) > 1e-6:
        add_track(board, copper, net, name, LAYERS[end_node[0]], end_node[0], w, ex, ey, tx, ty)
    sx, sy = xy(start_node[1])
    # the start sits in a pad or on an existing track of this net; join it to the pad centre if it's a pad
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname() != name:
                continue
            px, py = to_mm(pad.GetPosition().x), to_mm(pad.GetPosition().y)
            c = pad_copper(pad)
            m = np.zeros((2, grid.h, grid.w), bool)
            c.block(grid, m, 0, -0.05)
            j, i = divmod(start_node[1], W)
            if m[0, j, i] and math.hypot(sx - px, sy - py) > 1e-6:
                add_track(board, copper, net, name, LAYERS[start_node[0]], start_node[0], w, sx, sy, px, py)
                return


def covered_cells(grid, pieces, w):
    """Grid cells whose centre lies well inside these tracks and vias, on their layers"""
    cells = set()
    for c in pieces:
        g = c.g
        m = np.zeros((2, grid.h, grid.w), bool)
        # well inside: a new track starting at the cell centre lands on this copper
        shrink = g["r"] - (w / 2) * 0.6 if g["x1"] != g["x2"] or g["y1"] != g["y2"] else 0
        for li in c.layers:
            c.block(grid, m, li, -shrink)
            for j, i in zip(*np.nonzero(m[li])):
                cells.add((li, int(j) * grid.w + int(i)))
    return cells


def clear_line(grid, blocked, own, a, b):
    """Whether a straight track from cell a to cell b stays on free cells (or this net's own copper)"""
    W = grid.w
    aj, ai = divmod(a, W)
    bj, bi = divmod(b, W)
    n = int(max(abs(bi - ai), abs(bj - aj)) * 3) + 1
    for k in range(n + 1):
        t = k / n
        x, y = ai + (bi - ai) * t, aj + (bj - aj) * t
        # the cells around the point, so the track's width can't slip between two blocked cells
        for ci in {math.floor(x), math.ceil(x)}:
            for cj in {math.floor(y), math.ceil(y)}:
                idx = cj * W + ci
                if blocked[idx] and idx not in own:
                    return False
    return True


def smooth(grid, blocked, own, cells):
    """Straighten a grid path: from each corner, go as far along the path as a straight line can reach"""
    out = [cells[0]]
    k = 0
    while k < len(cells) - 1:
        far = k + 1
        for m in range(len(cells) - 1, k + 1, -1):
            if clear_line(grid, blocked, own, cells[k], cells[m]):
                far = m
                break
        out.append(cells[far])
        k = far
    return out


def add_track(board, copper, net, name, layer, li, w, x1, y1, x2, y2):
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(pcbnew.VECTOR2I(pcbnew.FromMM(x1), pcbnew.FromMM(y1)))
    t.SetEnd(pcbnew.VECTOR2I(pcbnew.FromMM(x2), pcbnew.FromMM(y2)))
    t.SetWidth(pcbnew.FromMM(w))
    t.SetLayer(layer)
    t.SetNet(net)
    board.Add(t)
    copper.append(Copper(name, (li,), "capsule", x1=x1, y1=y1, x2=x2, y2=y2, r=w / 2))


if __name__ == "__main__":
    import sys
    sys.exit(1 if route() else 0)
