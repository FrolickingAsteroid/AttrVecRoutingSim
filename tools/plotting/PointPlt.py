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

import copyreg
import pickle
def _reduce_count(c):
    # repr looks like "count(5)" or "count(5, 2)"
    args = tuple(int(x) for x in repr(c)[6:-1].split(","))
    return itertools.count, args

copyreg.pickle(itertools.count, _reduce_count)

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 12,
    "axes.labelsize": 12,
    "xtick.labelsize": 11,
    "ytick.labelsize": 12,
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

# ──────────────────────────────────────────────────────────────────────
# Path
# ──────────────────────────────────────────────────────────────────────

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = {
    "clique": ("bclique", [4, 8, 16, 32, 64], [1,2,4, 8, 16, 32, 64]),
    "triangle": ("triangle", [4, 8, 16, 32]),
    "focus": ("focus", [4, 8, 16])
}

DATASET_ = "focus"

#PERIODS = ["1.0","4.0","16.0","32.0"]
PERIODS = ["1.0"]

FAMILY_STYLE = {
    "BGP": ("-", ["s"]),
    "BGP-RCN": ("-.", ["o"]),
    "RRP": ("--", ["v", "^", "<", ">", "P", "X"]),
    "RRP-N": (":", ["D", "p", "h", "*", "8", "d"]),
}

DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET_
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"
Proto_label = ["RRP", "RRP-N", "BGP", "BGP-RCN"]

PROTOCOL = {
    "bgp": "BGP",
    "rpbob": "RRP",
    "rcn": "BGP-RCN",
}



def get_family(label: str) -> str:
    if label.startswith("RRP-N"):
        return "RRP-N"
    if label.startswith("RRP"):
        return "RRP"
    return label


def period_of(label: str) -> float:
    m = re.search(r"Δ = (\d+(?:\.\d+)?)", label)
    return float(m.group(1)) if m else 0.0


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        if "notif" in filename:
                return rf"RRP-N"
        else:
                return rf"RRP"

    return PROTOCOL.get(key, protocol_raw.upper())


rows = []
for csv_path in DATA_DIR.glob(f"{DATASET[DATASET_][0]}_*/*/*convergence.csv"):
    dataset = csv_path.parents[1].name
    protocol = csv_path.parent.name
    filename = csv_path.name.lower()

    print(csv_path)

    n = int(re.findall(r"(\d+)", dataset)[0])

    if n not in DATASET[f"{DATASET_}"][1]:
        continue

    if protocol.lower() == "rpbob" and PERIODS and re.findall(r"\d+\.\d+", filename)[0] not in PERIODS:
        continue

    protocol_label = get_protocol_label(protocol, filename)

    if protocol_label not in Proto_label:
        continue

    if protocol.lower() == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if delta not in PERIODS:
            continue
        if len(PERIODS) >= 1:
            protocol_label = protocol_label.ljust(6) + rf" Δ = {delta} ms"

    df = pd.read_csv(csv_path)
    df = df[df["change_count"] != 0]

    df = df[['stab_time']].copy()
    df[f"{DATASET_}"] = n
    df['protocol'] = protocol_label
    rows.append(df)

clique_data = pd.concat(rows, ignore_index=True)
clique_data["family"] = clique_data["protocol"].apply(get_family)

labels = clique_data["protocol"].unique().tolist()
families = ["BGP", "BGP-RCN", "RRP", "RRP-N"]

hue_order = sorted(labels, key=lambda l: (families.index(get_family(l)), period_of(l)))
family_colors = dict(zip(families, sns.color_palette("colorblind", n_colors=len(families))))

palette = {label: family_colors[get_family(label)] for label in labels}

markers = {}
for fam in families:
    fam_labels = [l for l in hue_order if get_family(l) == fam]  # already sorted by period
    for marker, label in zip(itertools.cycle(FAMILY_STYLE[fam][1]), fam_labels):
        markers[label] = marker
linestyles = {label: FAMILY_STYLE[get_family(label)][0] for label in hue_order}

columnwidth = 345.0
fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.8))

avg_times = clique_data.groupby(["protocol", f"{DATASET_}"])["stab_time"].mean()
print(avg_times)

sns.pointplot(
    data=clique_data,
    x=f"{DATASET_}",
    y="stab_time",
    hue="protocol",
    order=DATASET[DATASET_][1],
    hue_order=hue_order,
    palette=palette,
    markers=[markers[label] for label in hue_order],  # one per hue level, in hue_order
    linestyle=[linestyles[label] for label in hue_order],
    errorbar=None,
    dodge=0.3,
    markersize=8,
    linewidth=1,
    capsize=0.1,
)

ax.set_yscale('log')
#plt.yscale('symlog', linthresh=0.1)
ax.set_xlabel("Number of Nodes in Clique Topology")
ax.set_ylabel("Stabilization time [ms]")

# ax.set_yticks(DATASET[f"{DATASET_}"][2])
# ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
# ax.grid(alpha=0.3, which="major")

handles, labels = ax.get_legend_handles_labels()
legend = ax.legend(handles=handles, labels=labels)
new_labels = [rf"{label}" for label in labels]
for text in legend.get_texts():
    text.set_fontfamily("monospace")

plt.tight_layout()

name = f"{DATASET_}PointPlot_unit.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
pickle.dump(fig,open(f'raw_plots/{DATASET_}PointPlot_unit','wb'))
print(f"\tsaved {out_path}\n")
plt.show()
