#!/usr/bin/env python3
from pyexpat import error

import os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

plt.rcParams.update(
    {
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "font.size": 9,
    }
)

OUTPUT_DIR = os.path.join("outputs")
os.makedirs(OUTPUT_DIR, exist_ok=True)

file = "DelayLength"
metric = "lengths"
mode = "equal"
node_counts = [4, 8, 16, 32, 64]
protocol_paths = {
    r"RPBOB $\Delta = 1.0$": f"../../data/output/clique/bclique_n{{n}}/rpbob/{file}_{mode}_A_{metric}_N_adaptative_S_linkFailure_period_1.0_convergence.csv",
    r"RPBOB-RCN $\Delta = 1.0$": f"../../data/output/clique/bclique_n{{n}}/rpbob/{file}_{mode}_A_{metric}_N_adaptative_S_linkFailure_notif_period_1.0_convergence.csv",
    "BGP": f"../../data/output/clique/bclique_n{{n}}/bgp/{file}_{mode}_A_{metric}_S_linkFailure_convergence.csv",
    "BGP-RCN": f"../../data/output/clique/bclique_n{{n}}/rcn/{file}_{mode}_A_{metric}_S_linkFailure_convergence.csv",
}

frames = []
for n in node_counts:
    for protocol, path_template in protocol_paths.items():
        d = pd.read_csv(path_template.format(n=n))
        d = d[d["change_count"] > 0]
        d["Protocol"] = protocol
        d["n"] = n
        frames.append(d)
df = pd.concat(frames, ignore_index=True)

summary = (
    df.groupby(["n", "Protocol"])["stab_time"]
    .agg(mean="mean", min="min", max="max")
    .round(2)
    .reset_index()
)
print(summary.to_string(index=False))

plt.figure(figsize=(10, 4))

if mode == "unit":
    ax = sns.barplot(
        x="n",
        y="stab_time",
        data=df,
        hue="Protocol",
        errorbar=None
    )
    for container in ax.containers:
        ax.bar_label(container, fmt='%.1f', padding=3, fontsize=8)
else:
    ax = sns.boxenplot(
        x="n",
        y="stab_time",
        data=df,
        hue="Protocol",
        showfliers=True,
        k_depth="full",
    )
    for container in ax.containers:
        ax.bar_label(container, fmt='%.1f', padding=3, fontsize=8)

plt.xlabel('number of nodes')
plt.ylabel('Stabilization Time [ms]')
plt.grid(alpha=0.3, axis='y')
plt.tight_layout()

out_path = os.path.join(OUTPUT_DIR, f"boxen_clique_{mode}.png")
plt.savefig(out_path, dpi=300)
plt.close()
print(f"saved {out_path}")
