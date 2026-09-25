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

plt.rcParams.update({
    "font.family": "Liberation Sans",
    "mathtext.fontset": "cm",
    "font.size": 10,
})

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
    "clique": ("bclique", [4, 8, 16, 32], [1, 2, 10, 16, 32]),
    "triangle": ("triangle", [4, 8, 16, 32])
}

DATASET_ = "clique"

DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET_
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

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


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if "notif" in filename:
            return rf"RRP-N $\Delta = {delta}$ ms"
        else:
            return rf"RRP $\Delta = {delta}$ ms"

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

    if protocol.lower() == "rpbob" and re.findall(r"\d+\.\d+", filename)[0] != "1.0":
        continue

    protocol_label = get_protocol_label(protocol, filename)

    df = pd.read_csv(csv_path)
    df = df[df["change_count"] != 0]

    df = df[['stab_time']].copy()
    df[f"{DATASET_}"] = n
    df['protocol'] = protocol_label
    rows.append(df)

clique_data = pd.concat(rows, ignore_index=True)
clique_data["family"] = clique_data["protocol"].apply(get_family)

labels = clique_data["protocol"].unique().tolist()
families = ["BGP", "BGP-RCN", "RRP", "RRP-N", ]

hue_order = sorted(labels, key=lambda l: (families.index(get_family(l)), l))
family_colors = dict(zip(families, sns.color_palette("colorblind", n_colors=len(families))))

palette = {label: family_colors[get_family(label)] for label in labels}

marker_cycle = ['o', 's', '^', 'D', 'v', 'P', 'X', '*']
markers = {}

for fam in ["RRP-N", "RRP"]:
    fam_labels = sorted(l for l in labels if get_family(l) == fam)
    for marker, label in zip(itertools.cycle(marker_cycle), fam_labels):
        markers[label] = marker

markers["BGP"] = "D"
markers["BGP-RCN"] = "v"

fig, ax = plt.subplots(figsize=(6, 5))

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
    markers=['s', 'o', "v", 'D'],
    linestyle=['-', '-.', '--', ':'],
    errorbar=None,
    dodge=0.3,
    markersize=8,
    linewidth=1,
    capsize=0.1,
)

ax.set_yscale('log')

ax.set_xlabel("Number of Nodes in Clique Topology")
ax.set_ylabel("Stabilization time [ms]")

ax.set_yticks(DATASET[f"{DATASET_}"][2])
ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())

handles, labels = ax.get_legend_handles_labels()
ax.legend(handles=handles[1:], labels=labels[1:])
new_labels = [rf"{label}" for label in labels]

plt.tight_layout()

name = f"{DATASET_}PointPlot_unit.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
print(f"\tsaved {out_path}\n")
plt.show()
