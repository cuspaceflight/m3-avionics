# Microstrip patch antenna array drawing script.
# Units are metres/kilograms/seconds
# Axis has origin in the top left, x increasing right, y increasing down
# Copyright 2016 Adam Greig

from __future__ import print_function, division
import datetime
import numpy as np
from scipy.integrate import quad
from scipy.special import jn
from sexp import generate

# Target frequencies (for reference only)
# GPS: 1575.42MHz
# Telemetry: 869.5MHz

# Board constants (for reference only)
# dielectric constant: 2.67

# Microstrip line widths for various impedances [m]
strip_w_50r = 1.90e-3
strip_w_70r71 = 1.07e-3
strip_w_100r = 0.527e-3
strip_w_112r91 = 0.39e-3

# Feed networks. Entries are (width [m], length [m]).
# Lengths of None imply "continue to junction".
feed_booster_gps = [
    [  # Patch to first T-junction
        [  # Vertical section
            (strip_w_112r91, 22.32e-3),  # Part of L/4, 255 to 100R
        ],

        [  # Horizontal section
            (strip_w_112r91, 11.25e-3),  # Rest of L/4, 255 to 100R
            (strip_w_70r71, None),  # Complete L/4, 100 to 50R.
                                    # Will be 32.73e-3 long.
        ],
    ],

    [  # Subsequent T-junction to T-junction
        [  # Vertical section
            (strip_w_70r71, 20e-3),  # Part of L/4, 50 to 100R
        ],

        [  # Horizontal section
            (strip_w_70r71, 12.73e-3),  # Rest of L/4, 50 to 100R
            (strip_w_100r, None),  # Normal stripline for remaining length
        ],
    ]
]

feed_booster_telemetry = [
    [  # Patch to first T-junction at feedpoint
        [  # Vertical section
            (strip_w_100r, 50e-3),  # Plain 100R feedline
        ],

        [  # Horizontal section
            (strip_w_100r, None),  # Plain 100R feedline
        ]
    ],
    [[], []]
]

feed_dart_gps = [
    [  # Patch to first T-junction at feedpoint
        [  # Vertical
            (strip_w_112r91, 31.75e-3),  # Part of L/4 255 to 50R
        ],

        [  # Horizontal
            (strip_w_112r91, 1.82e-3),  # Rest of L/4 255 to 50R
            (strip_w_70r71, None),  # Complete L/4 50 to 100R
                                    # Will be 32.73e-3 long
        ]
    ],
    [[], []]
]

feed_dart_telemetry = [
    [  # Patch to feed directly
        [  # Vertical
            (strip_w_50r, 50e-3),  # Plain 50R feedline up to the feedpoint
        ],
        [],
    ],
    [[], []]
]

# Antennas to make
# Each should specify:
# w_array: width of final array [m]
# w_patch: patch element width [m]
# l_patch: patch element length [m]
# w_inset: patch element inset width [m]
# l_inset: patch element inset length [m]
# r_corner: patch element corner radius, if truncated [m]
# h: dielectric height [m]
# feed: a feed network spec, as described above
antennas = [
    # Dart telemetry
    {
        "w_array": 44e-3 * np.pi,
        "feed": feed_dart_telemetry,
        "w_patch": 116e-3,
        "l_patch": 104e-3,
        "w_inset": 10e-3,
        "l_inset": 37e-3,
        "r_corner": None,
        "h": 0.7e-3,
    },
    # Dart GPS
    {
        "w_array": 44e-3 * np.pi,
        "feed": feed_dart_gps,
        "w_patch": 57.48e-3,
        "l_patch": 57.48e-3,
        "w_inset": 0,
        "l_inset": 0,
        "r_corner": 2e-3,
        "h": 0.7e-3,
    },
    # Booster telemetry
    {
        "w_array": 112e-3 * np.pi,
        "feed": feed_booster_telemetry,
        "w_patch": 114e-3,
        "l_patch": 104e-3,
        "w_inset": 10e-3,
        "l_inset": 30e-3,
        "r_corner": None,
        "h": 0.7e-3,
    },
    # Booster GPS
    {
        "w_array": 112e-3 * np.pi,
        "feed": feed_booster_gps,
        "w_patch": 57.48e-3,
        "l_patch": 57.48e-3,
        "w_inset": 0,
        "l_inset": 0,
        "r_corner": 2e-3,
        "h": 0.7e-3,
    },
    # Tuned flat GPS patch at e=2.67
    {
        "w_array": 22e-3 * np.pi,
        "w_patch": 57.48e-3,
        "l_patch": 57.48e-3,
        "w_inset": 0e-3,
        "l_inset": 0e-3,
        "r_corner": 2e-3,
        "h": 0.7e-3,
        "feed": [[[(strip_w_112r91, 33.57e-3)], []], [[], []]]
    },
    # Tuned flat telemetry patch at e=2.67 (still being tuned)
    {
        "w_array": 44e-3 * np.pi,
        "w_patch": 114e-3,
        "l_patch": 104e-3,
        "w_inset": 5e-3,
        "l_inset": 37e-3,
        "r_corner": 0,
        "h": 0.7e-3,
        "feed": [[[(strip_w_50r, 50e-3)], []], [[], []]]
    },
]

antennas = [

    # Booster GPS
    {
        "w_array": 500e-3,
        "feed": feed_booster_gps,
        "w_patch": 57.48e-3,
        "l_patch": 57.48e-3,
        "w_inset": 0,
        "l_inset": 0,
        "r_corner": 2e-3,
        "h": 0.7e-3,
    },

]


def patch_impedance(w, l, r, h, f0):
    """
    Compute an estimate of the patch impedance, for patch of width w and length
    l, inset feed at distance r, and height above ground h, at frequency f0.
    """
    c0 = 299792458
    l0 = c0 / f0
    k0 = 2 * np.pi / l0
    si = lambda x: quad(lambda t: np.sin(t)/t, 0, x)[0]
    x = k0 * w
    i1 = -2 + np.cos(x) + x * si(x) + np.sin(x)/x
    g1 = i1 / (120 * np.pi**2)
    g12 = 1/(120*np.pi**2) * quad(
        lambda th: (np.sin((k0*w)/2 * np.cos(th))/np.cos(th))**2
        * jn(0, k0 * l * np.sin(th)) * np.sin(th)**3,
        0, np.pi)[0]
    rin0 = 1/(2*(g1 + g12))
    rin = rin0 * np.cos(np.pi * r / l)**2
    print("l0={:.4f} g1={:.4e} g12={:.4e} rin0={:.2f} rin={:.2f}"
          .format(l0, g1, g12, rin0, rin))
    return rin


def make_arrays(specs):
    """
    For each item in specs, generate the antenna array, and tile them
    vertically.
    Returns feedpoints, cutouts, and zones, suitable for generate_pcb.
    """
    y = 20e-3
    cutouts = []
    feedpoints = []
    zones = []
    for spec in specs:
        fp, co, p, s = make_array(spec)
        h = co[0][1] - co[1][1]
        y -= co[1][1]
        w = co[0][0] - co[2][0]
        x = -w/2 + 20e-3
        cutouts.append(translate(co, x, y))
        feedpoints.append((fp[0]+x, fp[1]+y))
        zones += [translate(patch, x, y) for patch in p]
        zones += [translate(strip, x, y) for strip in s]
        y += h/2 + 20e-3

    return feedpoints, cutouts, zones


def make_array(spec):
    """
    From an antenna specification, work out what the array should look like.
    """
    patches = patch_array(spec)
    n = len(patches)
    strips, height = microstrip_tree(spec, n)
    feedpoint = [0, -height + 0.1e-3]
    y1 = spec['l_patch']/2 + 5e-3
    y2 = -height - 5e-3
    w = spec['w_array']
    cutout = [(-w/2, y1), (-w/2, y2), (w/2, y2), (w/2, y1)]

    return feedpoint, cutout, patches, strips


def patch_array(spec):
    """
    Make an array of patches such that we have a power of two number of patches
    and they fill the width as best as possible.
    """
    w_body = spec['w_array']
    n = int(2**(np.floor(np.log2(w_body / spec['w_patch']))))
    spacing = w_body / n
    patch = generate_patch(spec['w_patch'], spec['l_patch'],
                           spec['w_inset'], spec['l_inset'],
                           spec['r_corner'])
    array = []
    for idx in range(n):
        array.append(translate(patch, idx*spacing - (n/2 - 1/2)*spacing, 0))

    return array


def microstrip_tree(spec, n):
    """
    Make a tree of microstrip to connect up antennas.
    Returns a list of microstrip patches, and the height of the centrepoint of
    the top layer of the tree.
    """
    feed = spec['feed']
    h = spec['h']
    spacing = spec['w_array'] / n
    y = -spec['l_patch']/2 + spec['l_inset']
    strips = []

    # Bypass the tree for a single patch with a direct feed
    if n == 1:
        w = feed[0][0][0][0]
        y1 = -spec['l_patch']/2 + spec['l_inset']
        y2 = y1 - feed[0][0][0][1]
        return [generate_feedline([(0, y1), (0, y2)], w, h)], -y2

    # Walk the tree, from the bottom up
    for layer in range(int(np.log2(n))):
        # Pick either the patch-to-first-T or the general T-to-T spec
        if layer == 0:
            lspec = feed[0]
        else:
            lspec = feed[1]

        # Consider each pair of child objects
        pairs = 2**(np.log2(n) - layer - 1)
        pairspace = spacing * 2**layer
        for i in range(int(pairs)):
            mid = (2*i + 1 - pairs) * pairspace
            x1 = mid - pairspace/2
            x2 = mid + pairspace/2
            p, y2 = microstrip_l(x1, mid, y, lspec, h)
            strips += p
            p, y2 = microstrip_l(x2, mid, y, lspec, h)
            strips += p

        y = y2

    return strips, -y


def microstrip_l(x, x2, y, lspec, h):
    """
    Draw a microstrip upside-down-L shape from x to x2, starting at y,
    and according to lspec (a list of two lists; the first a list of vertical
    segments and the second a list of horizontal segments, each segment with a
    width and a length).

    Returns a list of patches and the new height.
    """

    # Store generated strips
    strips = []

    # Emit a strip whenever the current width changes
    w = None

    # Store points to draw current strip between
    points = [(x, y)]

    # Compute the horizontal direction
    hdir = 1 if x2 > x else -1

    # Process each bit of vertical strip
    for vspec in lspec[0]:
        if w is None:
            w = vspec[0]
        elif w != vspec[0]:
            strips.append(generate_feedline(points, w, h))
            points = [(x, y)]
            w = vspec[0]
        y -= vspec[1]
        points.append((x, y))

    # Process the horizontal strips
    for hspec in lspec[1]:
        if w is None:
            w = hspec[0]
        elif w != hspec[0]:
            strips.append(generate_feedline(points, w, h))
            points = [(x, y)]
            w = hspec[0]
        dx = hspec[1]
        if dx is None:
            dx = abs(x2 - x)
        x += dx * hdir
        points.append((x, y))

    # Generate last strip
    strips.append(generate_feedline(points, w, h))

    return strips, y


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
        v = direction(pa, pb)
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
            ["min_thickness", 0.01],
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
        ["page", "A0"],
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


if __name__ == "__main__":
    feedpoints, cutouts, zones = make_arrays(antennas)
    pcb = generate_pcb(feedpoints, cutouts, zones, [])

    with open("test_patch.kicad_pcb", "w") as f:
        f.write(pcb)
