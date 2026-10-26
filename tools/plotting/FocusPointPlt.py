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
DODGE = 0.45  # protocols with equal values are shifted sideways so their markers don't overlap
METRIC = "time"  # time: stabilisation time | changes: number of route changes during convergence
COLUMN = {"time": "stab_time", "changes": "change_count"}

# same colours, line styles and markers as PointPlt.py; the dict order is the left-to-right order
FAMILIES = ["BGP", "BGP-RCN", "RRP", "RRP-N"]
FAMILY_STYLE = {
    "BGP": ("-", "s"),
    "BGP-RCN": ("-.", "o"),
    "RRP-N": (":", "D"),
    "RRP": ("--", "v"),
}

# BGP-RCN and RRP-N stabilise exactly like BGP on focus: their points reuse BGP's values,
# marked "(same as BGP)" in the printed table
SAME_AS_BGP = ["BGP-RCN", "RRP-N"]

# RRP on focus_n32 runs out of memory, so its point is estimated (and flagged in the printed table):
#   time:    2 * BGP - 1, exact for n = 4, 8 and 16 (BGP = 2^(n-1), RRP = 2^n - 1)
#   changes: n - 1, one change per path in RRP's window (3, 7 and 15 for n = 4, 8 and 16)
EXTRAPOLATE = [("RRP", 32)]
ESTIMATE = {
    "time": ("(2 * BGP - 1)", lambda bgp, n: 2 * bgp - 1),
    "changes": ("(n - 1)", lambda bgp, n: n - 1),
}

parser = argparse.ArgumentParser(description="Plot the stabilisation time, or the number of route changes, of one "
                                             "node of the focus topology.")
parser.add_argument("-m", "--metric", choices=list(COLUMN), default=METRIC,
                    help="time: stabilisation time; changes: route changes during convergence (default: %(default)s)")
parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
parser.add_argument("--node", type=int, default=NODE, help=f"default: {NODE}")
parser.add_argument("--period", default=PERIOD, help=f"RRP period (default: {PERIOD})")
parser.add_argument("--sizes", nargs="+", type=int, default=SIZES, help=f"default: {' '.join(map(str, SIZES))}")
parser.add_argument("--no-estimate", dest="estimate", action="store_false", help="don't add the estimated RRP point")
args = parser.parse_args()

SCENARIO, NODE, PERIOD, SIZES, METRIC = args.scenario, args.node, args.period, args.sizes, args.metric
if not args.estimate:
    EXTRAPOLATE = []


def get_family(protocol: str, filename: str):
    if protocol == "bgp":
        return "BGP"
    if protocol == "rcn":
        return "BGP-RCN"
    if protocol == "rpbob":
        return "RRP-N" if "_notif_" in filename else "RRP"
    return None


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

    rows.append({"family": family, "n": n, "value": df[COLUMN[METRIC]].max(), "estimated": False,
                 "priority": 0 if len(parts) == 4 else 1, "file": str(csv_path.relative_to(DATA_DIR))})

data = (pd.DataFrame(rows)
        .sort_values("priority")
        .drop_duplicates(["family", "n"])
        .drop(columns="priority"))

bgp_rows = data[data["family"] == "BGP"]
data = pd.concat([data[~data["family"].isin(SAME_AS_BGP)]] +
                 [bgp_rows.assign(family=family, estimated=True, file="(same as BGP)") for family in SAME_AS_BGP],
                 ignore_index=True)

for family, n in EXTRAPOLATE:
    measured = data[(data["family"] == family) & (data["n"] == n)]
    bgp = data[(data["family"] == "BGP") & (data["n"] == n)]
    if not measured.empty or bgp.empty:
        continue
    source, estimate = ESTIMATE[METRIC]
    data = pd.concat([data, pd.DataFrame([{"family": family, "n": n, "value": estimate(bgp["value"].iloc[0], n),
                                           "estimated": True, "file": source}])], ignore_index=True)

data = data.sort_values(["family", "n"])
print(data.to_string(index=False))

colors = dict(zip(FAMILIES, sns.color_palette("colorblind", n_colors=len(FAMILIES))))
present = [f for f in FAMILY_STYLE if f in data["family"].unique()]
offsets = dict(zip(present, np.linspace(-DODGE / 2, DODGE / 2, len(present)) if len(present) > 1 else [0.0]))

columnwidth = 345.0
fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.8))

handles, labels = [], []
for family in present:
    group = data[data["family"] == family]
    x = np.array([SIZES.index(n) for n in group["n"]]) + offsets[family]
    linestyle, marker = FAMILY_STYLE[family]
    style = dict(color=colors[family], linestyle=linestyle, linewidth=1, marker=marker, markersize=7)

    ax.plot(x, group["value"], **style)

    handles.append(Line2D([], [], **style))
    labels.append(family.ljust(6) + f"Δ = 1.0 ms" if family.startswith("RRP") else family)

ax.set_yscale('log')
if METRIC == "time":
    ax.yaxis.set_major_locator(matplotlib.ticker.FixedLocator([10 ** k for k in range(0, 11, 2)]))
    ax.yaxis.set_major_formatter(matplotlib.ticker.LogFormatterMathtext())
    ax.set_ylim(data["value"].min() / 3, data["value"].max() * 5)
    ax.set_ylabel(fr"Stabilization time of node $u$ [ms]")
else:
    ax.yaxis.set_major_locator(matplotlib.ticker.FixedLocator([1, 2, 4, 8, 16, 32]))
    ax.yaxis.set_major_formatter(matplotlib.ticker.ScalarFormatter())
    ax.set_ylim(0.7, data["value"].max() * 1.6)
    ax.set_ylabel(fr"Attribute changes of node $u$")
ax.grid(alpha=0.3, which="major")

legend = ax.legend(handles=handles, labels=labels, loc="upper left")
for text in legend.get_texts():
    text.set_fontfamily("monospace")

ax.set_xticks(range(len(SIZES)), [str(n) for n in SIZES])
ax.set_xlim(-0.5, len(SIZES) - 0.5)
ax.set_xlabel("Number of nodes in Focus topology")

plt.tight_layout()

name = f"focusNode{NODE}_{SCENARIO}.pdf" if METRIC == "time" else f"focusNode{NODE}_{SCENARIO}_changes.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
print(f"\tsaved {out_path}\n")
plt.show()
