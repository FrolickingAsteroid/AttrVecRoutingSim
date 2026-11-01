#!/usr/bin/env python3


from pathlib import Path
import seaborn as sns
import pandas as pd

import matplotlib.pyplot as plt
from matplotlib import rc
import matplotlib.ticker
import matplotlib

import itertools
import glob
import re
import os

# sized for LaTeX as in https://scipy-cookbook.readthedocs.io/items/Matplotlib_LaTeX_Examples.html:
LINEWIDTH = 345.0

SIDE_BY_SIDE = True
WF, HF = (1, 0.85) if SIDE_BY_SIDE else (1.0, (5. ** 0.5 - 1.0) / 2.0)

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 10,
    "axes.labelsize": 12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 10,
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

# ──────────────────────────────────────────────────────────────────────
# Path
# ──────────────────────────────────────────────────────────────────────

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = {
    "clique": ("bclique", [4, 8, 16, 32], [1, 2, 4, 8, 16, 32]),
    "triangle": ("triangle", [4, 8, 16, 32])
}

DATASET_ = "clique"

PERIODS = ["1.0"]

FAMILY_STYLE = {
    "BGP": ("-", ["s"]),
    "BGP-RCN": ("-.", ["o"]),
    "SyRuP": ("--", ["v", "^", "<", ">", "P", "X"]),
    "SyRuP-LFN": (":", ["D", "p", "h", "*", "8", "d"]),
}

DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET_
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "bgp": "BGP",
    "rpbob": "SyRuP",
    "rcn": "BGP-RCN",
}


def get_family(label: str) -> str:
    if label.startswith("SyRuP-LFN"):
        return "SyRuP-LFN"
    if label.startswith("SyRuP"):
        return "SyRuP"
    return label


def period_of(label: str) -> float:
    m = re.search(r"Δ = (\d+(?:\.\d+)?)", label)
    return float(m.group(1)) if m else 0.0


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if "notif" in filename:
            return rf"SyRuP-LFN Δ = {delta} ms"
        else:
            return rf"SyRuP".ljust(10) + f"Δ = {delta} ms"

    return PROTOCOL.get(key, protocol_raw.upper())


rows = []
for csv_path in DATA_DIR.glob(f"{DATASET[DATASET_][0]}_*/*/*convergence.csv"):
    dataset = csv_path.parents[1].name
    protocol = csv_path.parent.name
    filename = csv_path.name.lower()

    n = int(re.findall(r"(\d+)", dataset)[0])

    if n not in DATASET[f"{DATASET_}"][1]:
        continue

    if "altered" in filename:
        continue

    if protocol.lower() == "rpbob" and PERIODS and re.findall(r"\d+\.\d+", filename)[0] not in PERIODS:
        continue

    protocol_label = get_protocol_label(protocol, filename)

    df = pd.read_csv(csv_path)
    df = df[df["change_count"] != 0]
    df = df[df["stab_time"] > 0]  

    df = df[['stab_time']].copy()
    df[f"{DATASET_}"] = n
    df['protocol'] = protocol_label
    rows.append(df)

clique_data = pd.concat(rows, ignore_index=True)
clique_data["family"] = clique_data["protocol"].apply(get_family)

labels = clique_data["protocol"].unique().tolist()
families = ["BGP", "BGP-RCN", "SyRuP", "SyRuP-LFN"]

hue_order = sorted(labels, key=lambda l: (families.index(get_family(l)), period_of(l)))
family_colors = dict(zip(families, sns.color_palette("colorblind", n_colors=len(families))))

palette = {label: family_colors[get_family(label)] for label in labels}

markers = {}
for fam in families:
    fam_labels = [l for l in hue_order if get_family(l) == fam]  # already sorted by period
    for marker, label in zip(itertools.cycle(FAMILY_STYLE[fam][1]), fam_labels):
        markers[label] = marker
linestyles = {label: FAMILY_STYLE[get_family(label)][0] for label in hue_order}

fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

stats = clique_data.groupby(["protocol", f"{DATASET_}"])["stab_time"].agg(["min", "mean", "max"])
print(stats)

sns.pointplot(
    data=clique_data,
    x=f"{DATASET_}",
    y="stab_time",
    hue="protocol",
    order=DATASET[DATASET_][1],
    hue_order=hue_order,
    palette=palette,
    estimator="max",                         # mean or max...
    errorbar=None,
    markers=[markers[label] for label in hue_order],
    linestyle=[linestyles[label] for label in hue_order],
    dodge=0.5,
    markersize=8,
    linewidth=1,
)

ax.set_yscale('log')

ax.set_xlabel("Nodes in Clique Topology" if SIDE_BY_SIDE else "Number of Nodes in Clique Topology")
ax.set_ylabel("Stabilization time [ms]")

ax.set_yticks(DATASET[f"{DATASET_}"][2])
ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
ax.grid(alpha=0.3, which="major")

handles, labels = ax.get_legend_handles_labels()
legend = ax.legend(handles=handles, labels=labels, loc="upper left")
new_labels = [rf"{label}" for label in labels]
for text in legend.get_texts():
    text.set_fontfamily("monospace")

plt.tight_layout()

name = f"{DATASET_}PointPlot_unit.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
print(f"\tsaved {out_path}\n")
plt.show()