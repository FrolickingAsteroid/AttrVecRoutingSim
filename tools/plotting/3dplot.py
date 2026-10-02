#!/usr/bin/env python3
import matplotlib.pyplot as plt
from matplotlib import rc
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
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

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "rocketfuel"
SCENARIO = "linkFailure"

DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

DELTA = re.compile(r"period_([\d.]+)")


def extract_delta(path: Path) -> float:
    m = DELTA.search(path.name)
    return float(m.group(1))


def load_avg_stab_time(path: Path) -> float:
    df = pd.read_csv(path)
    df = df[df["stab_time"] != 0]
    return df["stab_time"].mean()


def load_avg_wind_size(path: Path) -> float:
    df = pd.read_csv(path)
    return df["wind_size"].mean()


network = "1755"
wind_dir = DATA_DIR / network / "window"

wind_by_delta = {}
for wind_path in wind_dir.glob(f"*{SCENARIO}*period_*.csv"):
    delta = extract_delta(wind_path)
    wind_by_delta[delta] = load_avg_wind_size(wind_path)

rows = []
for stab_path in DATA_DIR.glob(f"{network}/rpbob/*{SCENARIO}*convergence.csv"):

    if "notif" in stab_path.name:
        continue

    delta = extract_delta(stab_path)
    avg_stab = load_avg_stab_time(stab_path)

    if delta not in wind_by_delta:
        print(f"skipping ({stab_path.name})")
        continue

    rows.append({
        "delta": delta,
        "avg_wind_size": wind_by_delta[delta],
        "avg_stab_time": avg_stab,
    })

data = pd.DataFrame(rows).sort_values("delta")
print(data)

fig, axes = plt.subplots(1, 2, figsize=(12, 4), sharex=True)

axes[0].plot(data["delta"], data["avg_wind_size"], marker="o", color="#0072B2")
axes[0].set_ylabel("Avg window size")
axes[0].grid(alpha=0.3)

axes[1].plot(data["delta"], data["avg_stab_time"], marker="o", color="#D55E00")
axes[1].set_ylabel("Avg stabilization time")
axes[1].grid(alpha=0.3)

fig.supxlabel(r"Period of Routing Computations, $\Delta$")

plt.tight_layout()
plt.savefig(OUTPUT_DIR / "delta_1755.png", dpi=300)
plt.show()
