# Microstrip patch antenna array drawing script.
# Units are metres/kilograms/seconds
# Axis has origin in the top left, x increasing right, y increasing down
# Copyright 2016 Adam Greig

from __future__ import print_function, division
import datetime
import numpy as np
from sexp import generate

# Constants
c0 = 299792458
feed_space = 10e-3

# Antennas to make
# Each should specify:
# d: diameter of conformed antenna [m]
# f: centre frequency of antenna [Hz]
# w: patch element width [m]
# l: patch element length [m]
# w_inset: patch element inset width [m]
# l_inset: patch element inset length [m]
# r_corner: patch element corner radius, if truncated [m]
# h: height of dielectric [m]
# e_r: relative permittivity of dielectric
# t_copper: thickness of the copper layer [m]
antennas = {
    "test": {
        "d": 300e-3,
        "w": 50e-3,
        "l": 50e-3,
        "w_inset": 10e-3,
        "l_inset": 10e-3,
        "r_corner": 10e-3,
        "h": 0.7e-3,
        "e_r": 3.4,
        "t_copper": 35e-6,
    },
    "dart-telem": {
        "d": 44e-3,
        "f": 869.5e6,
        "w": 116e-3,
        "l": 104e-3,
        "w_inset": 10e-3,
        "l_inset": 20e-3,
        "r_corner": None,
        "h": 0.7e-3,
        "e_r": 3.4,
        "t_copper": 35e-6,
    },
    "dart-gps": {
        "d": 44e-3,
        "f": 1575.42e6,
        "w": 64e-3,
        "l": 52e-3,
        "w_inset": 10e-3,
        "l_inset": 10e-3,
        "r_corner": 10e-3,
        "h": 0.7e-3,
        "e_r": 3.4,
        "t_copper": 35e-6,
    },
    "booster-telem": {
        "d": 112e-3,
        "f": 869.5e6,
        "w": 116e-3,
        "l": 104e-3,
        "w_inset": 10e-3,
        "l_inset": 20e-3,
        "r_corner": None,
        "h": 0.7e-3,
        "e_r": 3.4,
        "t_copper": 35e-6,
    },
    "booster-gps": {
        "d": 112e-3,
        "f": 1575.42e6,
        "w": 64e-3,
        "l": 52e-3,
        "w_inset": 10e-3,
        "l_inset": 10e-3,
        "r_corner": 10e-3,
        "h": 0.7e-3,
        "e_r": 3.4,
        "t_copper": 35e-6,
    }
}


def make_array(spec):
    """
    From an antenna specification, work out what the array should look like.
    """
    patches = patch_array(spec)
    n = len(patches)
    strips = microstrip_tree(spec, n)

    return patches, strips


def patch_array(spec):
    """
    Make an array of patches such that we have a power of two number of patches
    and they fill the width as best as possible.
    """
    w_body = np.pi * spec['d']
    n = int(2**(np.floor(np.log2(w_body / spec['w']))))
    spacing = w_body / n
    patch = generate_patch(spec['w'], spec['l'],
                           spec['w_inset'], spec['l_inset'],
                           spec['r_corner'])
    array = []
    for idx in range(n):
        array.append(translate(patch, idx*spacing - (n/2 - 1/2)*spacing, 0))

    return array


def microstrip_tree(spec, n):
    """
    Make a tree of microstrip to connect up antennas.
    """
    # Compute microstrip width for 50R match
    # TODO this is probably not the best match or the best formula
    h = spec['h']
    er = spec['e_r']
    t_copper = spec['t_copper']
    w_f = (7.48 * h)/(np.exp(np.sqrt(0.33 * (er + 1.41)))) - 1.25 * t_copper

    spacing = np.pi * spec['d'] / n

    strips = []

    for layer in range(int(np.log2(n))):
        y1 = -spec['l']/2 - (layer+1)*feed_space
        y2 = y1 + feed_space
        if layer == 0:
            y2 = 0.0
        pairs = 2**(np.log2(n) - layer - 1)
        pairspace = spacing * 2**layer
        for i in range(int(pairs)):
            mid = (2*i + 1 - pairs) * pairspace
            x1 = mid - pairspace/2
            x2 = mid + pairspace/2
            strips.append(generate_feedline(
                [(x1, y2), (x1, y1), (x2, y1), (x2, y2)], w_f, h))

    return strips


def direction(pa, pb):
    """Find the direction between two points."""
    v = pb[0] - pa[0], pb[1] - pa[1]
    l = np.sqrt(v[0]**2 + v[1]**2)
    return v[0]/l, v[1]/l


def convex(v1, v2):
    """See if (v1, v2) are convex."""
    return (-v1[1] * v2[0] + v1[0]*v2[1]) < 0


def generate_feedline(points, w, h):
    """
    Generate microstrip feedline passing through points of width w [m].
    Specify the height above dielectric h [m] for mitreing purposes.
    Returns a zone definition (list of zone corners).
    """

    assert len(points) >= 2, "Must have at least two points to draw feedline"

    # Cop out if there are only two points (easy mode)
    if len(points) == 2:
        pa, pb = points
        v, l = direction(pa, pb)
        return [
            (pa[0] + v[1]*w/2, pa[1] + v[0]*w/2),
            (pb[0] + v[1]*w/2, pb[1] + v[0]*w/2),
            (pb[0] - v[1]*w/2, pb[1] - v[0]*w/2),
            (pa[0] - v[1]*w/2, pa[1] - v[0]*w/2),
            (pa[0] + v[1]*w/2, pa[1] + v[0]*w/2)
        ]

    # Compute mitre related constants for three or more points (hard mode)
    assert w/h >= 1/4, "Microstrip W/H must >= 1/4 for mitres to be correct"
    m = (52 + 65 * np.exp(-27/20 * w/h)) / 100
    d = w * np.sqrt(2)
    x = d * m
    a = x * np.sqrt(2)

    line = []

    def emit_corner(pa, pb, pc):
        """
        Compute where to stick a zone corner for the three points along a line.
        The corner will be near pb, but is mostly w/2 away from it to get the
        right track width.
        For concave corners, we simply stick the corner at the point which is
        w/2 away from pb perpendicular to pa->pb and w/2 away in the pb->pa
        direction (in other words, this is the inside corner in the pa-pb-pc
        angle, a distance sqrt(2)*w/2 from pb).
        For convex corners we must mitre, so take the previous point and first
        pull it in the pb->pa direction by a, and then make a second point
        pushed in the pb->pc direction (perpendicular to pb->pa) by a.
        The parameter a is as per Douville and James "Experimental study of
        symmetric microstrip bends and their compensation", 1978, IEEE Trans
        Microwave Theory Tech.
        """
        v1, v2 = direction(pa, pb), direction(pb, pc)
        if convex(v1, v2):
            line.append((pb[0] - v1[1]*w/2 + v1[0]*(w/2 - a),
                         pb[1] + v1[1]*(w/2 - a) + v1[0]*w/2))
            line.append((pb[0] - v1[1]*(w/2 - a) + v1[0]*w/2,
                         pb[1] + v1[1]*w/2 + v1[0]*(w/2 - a)))
        else:
            line.append((pb[0] - v1[1]*w/2 - v1[0]*w/2,
                         pb[1] - v1[1]*w/2 + v1[0]*w/2))

    # Starting corner by the first point
    v = direction(points[0], points[1])
    line.append((points[0][0] - v[1]*w/2, points[0][1] + v[0]*w/2))

    # Run in the forward direction
    for pa, pb, pc in zip(points, points[1:], points[2:]):
        emit_corner(pa, pb, pc)

    # Two corners at the end of the run
    v = direction(points[-2], points[-1])
    line.append((points[-1][0] - v[1]*w/2, points[-1][1] + v[0]*w/2))
    line.append((points[-1][0] + v[1]*w/2, points[-1][1] - v[0]*w/2))

    # Now run back to the start, doing the other side of the track
    for pa, pb, pc in zip(points[::-1], points[-2::-1], points[-3::-1]):
        emit_corner(pa, pb, pc)

    # Final point and close the loop
    v = direction(points[0], points[1])
    line.append((points[0][0] + v[1]*w/2, points[0][1] - v[0]*w/2))
    line.append((points[0][0] - v[1]*w/2, points[0][1] + v[0]*w/2))

    return line


def generate_patch(w, l, w_inset=None, l_inset=None, r_corner=None):
    """
    Draw a patch of width w [m] and length l [m],
    with optional inset feedpoint width w_inset [m] and length l_inset [m],
    and optional opposite corner truncation of length r_corner [m].
    Returns a zone definition (list of zone corners).
    """

    patch = [(-w/2, -l/2)]

    if r_corner:
        patch.append((-w/2, l/2 - r_corner))
        patch.append((-w/2 + r_corner, l/2))
    else:
        patch.append((-w/2, l/2))

    patch.append((w/2, l/2))

    if r_corner:
        patch.append((w/2, -l/2 + r_corner))
        patch.append((w/2 - r_corner, -l/2))
    else:
        patch.append((w/2, -l/2))

    if w_inset and l_inset:
        patch.append((w_inset/2, -l/2))
        patch.append((w_inset/2, -l/2 + l_inset))
        patch.append((-w_inset/2, -l/2 + l_inset))
        patch.append((-w_inset/2, -l/2))

    patch.append((-w/2, -l/2))

    return patch


def translate(points, x, y):
    """
    Translate a list of points by x and y.
    """
    return [(p[0]+x, p[1]+y) for p in points]


def scale(points, s):
    """
    Scale a list of points by s.
    """
    return [(p[0]*s, p[1]*s) for p in points]


def pcb_lines(points, layer, w=0.1):
    """
    Draw connected lines through points on layer.
    """
    lines = []
    for p, pp in zip(points, points[1:]):
        lines.append([
            "gr_line",
            ["start", p[0], p[1]],
            ["end", pp[0], pp[1]],
            ["layer", layer], ["width", w]
        ])

    return lines


def generate_pcb(feedpoints, cuts, zones, drawings):
    """
    Generate a KiCAD PCB file, with PTH pads located at the feedpoints,
    rectangular board edges defined by cuts, copper zones (which must intersect
    a feedpoint to be filled) defined by lists of points in zones, and drawings
    (list of lists of points).
    """
    # Rescale inputs to millimetres for KiCAD
    feedpoints = scale(feedpoints, 1e3)
    cuts = [scale(cut, 1e3) for cut in cuts]
    zones = [scale(z, 1e3) for z in zones]
    drawings = [scale(d, 1e3) for d in drawings]

    # Draw feedpoints
    modules = []
    for idx, feedpoint in enumerate(feedpoints):
        modules += [
            "module",
            "X{}".format(idx),
            ["layer", "F.Cu"],
            ["tedit", 0],
            ["tstamp", 0],
            ["at", feedpoint[0], feedpoint[1]],
            ["pad",
                1, "thru_hole", "circle", ["at", 0, 0], ["size", 2.5, 2.5],
                ["drill", 1.5], ["layers", "F.Cu"], ["zone_connect", 2],
                ["net", 1, "Antennas"]]],

    # Draw cutouts
    edges = []
    for cut in cuts:
        edges += pcb_lines(cut + [cut[0]], "Edge.Cuts")

    # Draw zones
    kicad_zones = []
    for zone in zones:
        pts = ["pts"]
        for vertex in zone:
            pts.append(["xy", vertex[0], vertex[1]])
        kicad_zones.append([
            "zone",
            ["net", 1],
            ["net_name", "Antennas"],
            ["layer", "F.Cu"],
            ["tstamp", 0],
            ["hatch", "edge", 0.5],
            ["connect_pads", ["clearance", 0.3]],
            ["min_thickness", 0.2],
            ["fill",
                "yes",
                ["arc_segments", 32],
                ["thermal_gap", 0.3],
                ["thermal_bridge_width", 0.25]],
            ["polygon", pts]
        ])

    # Draw drawing lines
    kicad_drawings = []
    for drawing in drawings:
        kicad_drawings += pcb_lines(drawing, "Dwgs.User")

    # Render the PCB
    out = [
        "kicad_pcb",
        ["version", 4],
        ["host", "antennas.py", datetime.datetime.utcnow().isoformat()],
        ["page", "A2"],
        ["layers",
            [0, "F.Cu", "signal"],
            [31, "B.Cu", "signal", "hide"],
            [40, "Dwgs.User", "user"],
            [44, "Edge.Cuts", "user"]],
        ["net", 0, ""],
        ["net", 1, "Antennas"],
        ["net_class", "Default", "",
            ["clearance", 0.2],
            ["trace_width", 0.2],
            ["via_dia", 0.8],
            ["via_drill", 0.4]],
    ] + modules + edges + kicad_drawings + kicad_zones
    return generate(out)


patches, strips = make_array(antennas['test'])
pcb = generate_pcb(
    [],
    [],
    patches + strips,
    []
)

with open("test_patch.kicad_pcb", "w") as f:
    f.write(pcb)
