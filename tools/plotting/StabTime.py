#!/usr/bin/env python3
import os
import re
import glob
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.family": "Liberation Sans",
    # "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
    "mathtext.fontset": "cm",
    "font.size": 10,
})

File = "DelayLength"
Mode = "placeholder"
Metric = "lengths"

DATA_ROOT = "../../data/output/rocketfuel"
# DATA_ROOT = "../../data/output/clique"
# DATASET_TAG = f"{File}_{Mode}_A_{Metric}"
DATASET_TAG = f"{File}_A_{Metric}"
SCENARIO = "normal"

OUTPUT_DIR = os.path.join("outputs")
os.makedirs(OUTPUT_DIR, exist_ok=True)

METRIC_SUFFIX = "convergence"

LABEL_RULES = [
    (r"rpbob.*notif", "RPBOB-CN"),
    (r"rpbob", "RPBOB"),
    (r"rcn", "BGP-RCN"),
    (r"bgp", "BGP"),
]


def label_for(protocol_folder, filename):
    key = f"{protocol_folder}_{filename}".lower()
    for pattern, label in LABEL_RULES:
        if re.search(pattern, key):
            return label
    return protocol_folder


def discover_networks(root):
    return sorted(d for d in os.listdir(root) if os.path.isdir(os.path.join(root, d)))


def load_metric(network_dir):
    frames = []
    pattern = os.path.join(network_dir, "*", f"{DATASET_TAG}*_{SCENARIO}*_{METRIC_SUFFIX}.csv")

    for path in sorted(glob.glob(pattern)):
        protocol_folder = os.path.basename(os.path.dirname(path))
        filename = os.path.basename(path)

        df = pd.read_csv(path)
        df = df[df["change_count"] != 0]
        df["Protocol"] = label_for(protocol_folder, filename)
        frames.append(df)
    if not frames:
        raise FileNotFoundError(f"no matching files under {network_dir}")
    return pd.concat(frames, ignore_index=True)


def plot_ccdf(df, network_name):
    df = df[df.Protocol != "BGP-RCN"]
    df = df[df["Protocol"] != "RPBOB-CN"]

    # Calculate average stab_time per protocol
    avg_times = df.groupby("Protocol")["stab_time"].mean()

    df["Protocol"] = df["Protocol"].map(
        lambda p: f"{p} (avg: {avg_times[p]:.2f} ms)"
    )

    plt.figure(figsize=(6, 4))

    ax = sns.ecdfplot(
        data=df,
        x="stab_time",
        hue="Protocol",
        complementary=True,
        lw=1.8,
    )
    ax.set_xlim([-0.0, df["stab_time"].max()])
    for line in ax.lines:
        x = line.get_xdata()
        x[0] = 0
        line.set_xdata(x)

    plt.xlabel("Time [ms]")
    plt.ylabel("CCDF")

    plt.ylim([-0.001, 1.1])
    plt.grid(alpha=0.3, which="both")
    plt.tight_layout()

    legend = ax.legend_
    legend.set_title("Protocol")
    legend.get_title().set_fontweight("bold")

    if Mode not in DATASET_TAG:
        name = f"ccdf_{network_name}_{SCENARIO}.png"
    else:
        name = f"ccdf_{network_name}_{Mode}_{SCENARIO}.png"

    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, dpi=300)
    plt.close()
    print(f"saved {out_path}")


for network in discover_networks(DATA_ROOT):
    network_dir = os.path.join(DATA_ROOT, network)
    try:
        df = load_metric(network_dir)
    except FileNotFoundError as e:
        print(f"skipping {network}: {e}")
        continue
    plot_ccdf(df, network)
