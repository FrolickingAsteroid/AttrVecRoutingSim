#!/usr/bin/env python3
"""Stabilisation time of a single node (default: node 0) in the focus topology, against the topology size."""

from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np

import matplotlib.pyplot as plt
import matplotlib.ticker
from matplotlib.lines import Line2D

import argparse
import re
import os

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 10,
    "axes.labelsize": 12,
    "xtick.labelsize": 11,
    "ytick.labelsize": 11,
    "legend.fontsize": 9,
})


# Source - https://stackoverflow.com/a/31527287
# Posted by rth, modified by community. See post 'Timeline' for change history
# Retrieved 2026-09-27, License - CC BY-SA 4.0

def get_figsize(columnwidth, wf=0.5, hf=(5. ** 0.5 - 1.0) / 2.0, ):
    """Parameters:
      - wf [float]:  width fraction in columnwidth units
      - hf [float]:  height fraction in columnwidth units.
                     Set by default to golden ratio.
      - columnwidth [float]: width of the column in latex. Get this from LaTeX
                             using \\showthe\\columnwidth
    Returns:  [fig_width,fig_height]: that should be given to matplotlib
    """
    fig_width_pt = columnwidth * wf
    inches_per_pt = 1.0 / 72.27  # Convert pt to inch
    fig_width = fig_width_pt * inches_per_pt  # width in inches
    fig_height = fig_width * hf  # height in inches
    return [fig_width, fig_height]


matplotlib.rcParams['ytick.minor.size'] = 0
matplotlib.rcParams['ytick.minor.width'] = 0

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATA_DIR = ROOT_DIR / 'data' / 'output' / 'focus'
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

SIZES = [4, 8, 16, 32]
SCENARIO = "cutset"  # also matches the files named after the cutset file ("cutset.tsv")
NODE = 0
PERIOD = "1.0"
DODGE = 0.3

# same colours, line styles and markers as PointPlt.py
FAMILIES = ["BGP", "BGP-RCN", "RRP", "RRP-N"]
FAMILY_STYLE = {
    "BGP": ("-", "s"),
    "RRP": ("--", "v"),
    "RRP-N": (":", "D"),
}

# BGP and BGP-RCN converge identically on focus: only BGP is read, and labelled as both
LABELS = {
    "BGP": "BGP / BGP-RCN",
    "RRP": "RRP",
    "RRP-N": "RRP-N",
}

# RRP on focus_n32 runs out of memory. Its point is estimated as 2 * BGP - 1, which holds exactly
# for n = 4, 8 and 16 (BGP = 2^(n-1), RRP = 2^n - 1). It is flagged in the printed table
EXTRAPOLATE = [("RRP", 32)]

parser = argparse.ArgumentParser(description="Plot the stabilisation time of one node of the focus topology.")
parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
parser.add_argument("--node", type=int, default=NODE, help=f"default: {NODE}")
parser.add_argument("--period", default=PERIOD, help=f"RRP period (default: {PERIOD})")
parser.add_argument("--sizes", nargs="+", type=int, default=SIZES, help=f"default: {' '.join(map(str, SIZES))}")
parser.add_argument("--no-estimate", dest="estimate", action="store_false", help="don't add the estimated RRP point")
parser.add_argument("--no-ratio", dest="ratio", action="store_false", help="don't draw the panel relative to BGP")
args = parser.parse_args()

SCENARIO, NODE, PERIOD, SIZES = args.scenario, args.node, args.period, args.sizes
if not args.estimate:
    EXTRAPOLATE = []


def get_family(protocol: str, filename: str):
    if protocol == "bgp":
        return "BGP"
    if protocol == "rpbob":
        return "RRP-N" if "_notif_" in filename else "RRP"
    return None  # rcn: identical to bgp


# runs are either directly in <protocol>/ or in <protocol>/<scenario>/; the latter are newer and win
paths = list(DATA_DIR.glob("focus_n*/*/*convergence.csv")) + list(DATA_DIR.glob(f"focus_n*/*/{SCENARIO}/*convergence.csv"))
scenario_re = re.compile(rf"_S_{re.escape(SCENARIO)}(\.tsv)?_")

rows = []
for csv_path in paths:
    parts = csv_path.relative_to(DATA_DIR).parts
    n = int(re.findall(r"focus_n(\d+)", parts[0])[0])
    protocol = parts[1].lower()
    filename = csv_path.name

    if n not in SIZES or not scenario_re.search(filename) or "_inv_" in filename:
        continue

    family = get_family(protocol, filename)
    if family is None:
        continue

    if protocol == "rpbob" and f"period_{PERIOD}_" not in filename:
        continue

    if csv_path.stat().st_size == 0:  # runs that ran out of memory leave empty files behind
        print(f"skipping empty {csv_path}")
        continue

    df = pd.read_csv(csv_path)
    df = df[df["node"] == NODE]
    if df.empty:
        continue

    rows.append({"family": family, "n": n, "stab_time": df["stab_time"].max(), "estimated": False,
                 "priority": 0 if len(parts) == 4 else 1, "file": str(csv_path.relative_to(DATA_DIR))})

data = (pd.DataFrame(rows)
        .sort_values("priority")
        .drop_duplicates(["family", "n"])
        .drop(columns="priority"))

for family, n in EXTRAPOLATE:
    measured = data[(data["family"] == family) & (data["n"] == n)]
    bgp = data[(data["family"] == "BGP") & (data["n"] == n)]
    if not measured.empty or bgp.empty:
        continue
    data = pd.concat([data, pd.DataFrame([{"family": family, "n": n, "stab_time": 2 * bgp["stab_time"].iloc[0] - 1,
                                           "estimated": True, "file": "(2 * BGP - 1)"}])], ignore_index=True)

bgp_time = data[data["family"] == "BGP"].set_index("n")["stab_time"]
data["ratio"] = data["stab_time"] / data["n"].map(bgp_time)

data = data.sort_values(["family", "n"])
print(data.to_string(index=False))

colors = dict(zip(FAMILIES, sns.color_palette("colorblind", n_colors=len(FAMILIES))))
present = [f for f in FAMILY_STYLE if f in data["family"].unique()]
offsets = dict(zip(present, np.linspace(-DODGE / 2, DODGE / 2, len(present)) if len(present) > 1 else [0.0]))

columnwidth = 345.0
if args.ratio:
    fig, (ax, ax_ratio) = plt.subplots(2, 1, sharex=True, gridspec_kw={"height_ratios": [2.2, 1]},
                                       figsize=get_figsize(columnwidth, wf=1.1, hf=1.05))
else:
    fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.8))
    ax_ratio = None

handles, labels = [], []
for family in present:
    group = data[data["family"] == family]
    x = np.array([SIZES.index(n) for n in group["n"]]) + offsets[family]
    linestyle, marker = FAMILY_STYLE[family]
    style = dict(color=colors[family], linestyle=linestyle, linewidth=1, marker=marker, markersize=7)

    ax.plot(x, group["stab_time"], **style)
    if ax_ratio is not None:
        ax_ratio.plot(x, group["ratio"], **style)

    handles.append(Line2D([], [], **style))
    labels.append(LABELS[family])

# symlog: the RRP-N times are 0, which a log axis can't show
ax.set_yscale('symlog', linthresh=1, linscale=0.5)
ax.yaxis.set_major_locator(matplotlib.ticker.FixedLocator([0] + [10 ** k for k in range(0, 11, 2)]))
ax.yaxis.set_major_formatter(
    matplotlib.ticker.FuncFormatter(lambda v, _: "0" if v == 0 else rf"$10^{{{int(round(np.log10(v)))}}}$"))
ax.set_ylim(-0.3, data["stab_time"].max() * 5)
ax.set_ylabel(f"Stabilization time\nof node {NODE} [ms]")
ax.grid(alpha=0.3, which="major")

legend = ax.legend(handles=handles, labels=labels, loc="upper left")
for text in legend.get_texts():
    text.set_fontfamily("monospace")

bottom = ax
if ax_ratio is not None:
    ax_ratio.axhline(1, color="0.5", linewidth=0.8, zorder=0)
    ax_ratio.set_ylim(-0.25, max(2.5, data["ratio"].max() + 0.3))
    ax_ratio.set_yticks([0, 1, 2])
    ax_ratio.set_ylabel("Relative\nto BGP")
    ax_ratio.grid(alpha=0.3, which="major")
    bottom = ax_ratio

bottom.set_xticks(range(len(SIZES)), [str(n) for n in SIZES])
bottom.set_xlim(-0.5, len(SIZES) - 0.5)
bottom.set_xlabel("Number of Nodes in Focus Topology")

plt.tight_layout()
fig.align_ylabels()

name = f"focusNode{NODE}_{SCENARIO}.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
print(f"\tsaved {out_path}\n")
plt.show()
