from __future__ import print_function, division
import datetime
from sexp import generate


def generate_pcb(feedpoint, corners, zones):
    edges = [
        [
            "gr_line",
            ["start", corners[0][0], corners[0][1]],
            ["end", corners[1][0], corners[1][1]],
            ["layer", "Edge.Cuts"], ["width", 0.1]
        ], [
            "gr_line",
            ["start", corners[1][0], corners[1][1]],
            ["end", corners[2][0], corners[2][1]],
            ["layer", "Edge.Cuts"], ["width", 0.1]
        ], [
            "gr_line",
            ["start", corners[2][0], corners[2][1]],
            ["end", corners[3][0], corners[3][1]],
            ["layer", "Edge.Cuts"], ["width", 0.1]
        ], [
            "gr_line",
            ["start", corners[3][0], corners[3][1]],
            ["end", corners[0][0], corners[0][1]],
            ["layer", "Edge.Cuts"], ["width", 0.1]
        ]
    ]
    kicad_zones = []
    for zone in zones:
        pts = ["pts"]
        for vertex in zone:
            pts.append(["xy", vertex[0], vertex[1]])
        kicad_zones.append([
            "zone",
            ["net", 1],
            ["net_name", "A"],
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
    out = [
        "kicad_pcb",
        ["version", 4],
        ["host", "antennas.py", datetime.datetime.utcnow().isoformat()],
        ["page", "A2"],
        ["layers",
            [0, "F.Cu", "signal"],
            [31, "B.Cu", "signal", "hide"],
            [44, "Edge.Cuts", "user"]],
        ["net", 0, ""],
        ["net", 1, "A"],
        ["net_class", "Default", "",
            ["clearance", 0.2],
            ["trace_width", 0.2],
            ["via_dia", 0.8],
            ["via_drill", 0.4],
            ["add_net", "A"]],
        ["module",
            "X1",
            ["layer", "F.Cu"],
            ["tedit", 0],
            ["tstamp", 0],
            ["at", feedpoint[0], feedpoint[1]],
            ["pad",
                1, "thru_hole", "circle", ["at", 0, 0], ["size", 2.5, 2.5],
                ["drill", 1.5], ["layers", "F.Cu"], ["zone_connect", 2],
                ["net", 1, "A"]]],
    ] + edges + kicad_zones
    return generate(out)

print(
    generate_pcb(
        (100, 100),
        ((90, 90), (90, 110), (110, 110), (110, 90)),
        [[[95, 95], [105, 95], [105, 105], [95, 105]]]
    )
)
