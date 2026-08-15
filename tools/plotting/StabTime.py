#!/usr/bin/env python3
import os
import re
import glob
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.family": "Liberation Sans",
    "mathtext.fontset": "cm",
    "font.size": 10,
})

File = "DelayLength"
Mode = "placeholder"
Metric = "lengths"
DATA_ROOT = "../../data/output/rocketfuel"
DATASET_TAG = f"{File}_A_{Metric}"
SCENARIO = "linkFailure"
OUTPUT_DIR = "../../data/plots"
os.makedirs(OUTPUT_DIR, exist_ok=True)
METRIC_SUFFIX = "convergence"

LABEL_RULES = [
    (r"rpbob.*notif", "RPBOB-CN"),
    (r"rpbob", "RPBOB"),
    (r"rcn", "BGP-RCN"),
    (r"bgp", "BGP"),
]

# Which protocol labels to plot. None = plot everything label_for() produces.
SELECTED_PROTOCOLS = None
#SELECTED_PROTOCOLS = ["RPBOB", "BGP"]

# Which RPBOB periods to plot. None = plot every period found.
SELECTED_PERIODS = None
SELECTED_PERIODS = [15.0,1.0]

# Regex used to pull the period value out of an RPBOB filename.
PERIOD_REGEX = re.compile(r"_period_([0-9]+(?:\.[0-9]+)?)_")


def label_for(protocol_folder, filename):
    key = f"{protocol_folder}_{filename}".lower()
    for pattern, label in LABEL_RULES:
        if re.search(pattern, key):
            return label
    return protocol_folder


def extract_period(filename):
    match = PERIOD_REGEX.search(filename)
    if match:
        return float(match.group(1))
    return None


def discover_networks(root):
    return sorted(d for d in os.listdir(root) if os.path.isdir(os.path.join(root, d)))


def load_metric(network_dir):
    frames = []
    pattern = os.path.join(network_dir, "*", f"{DATASET_TAG}*_{SCENARIO}*_{METRIC_SUFFIX}.csv")
    for path in sorted(glob.glob(pattern)):
        print(f"\t{path}")
        protocol_folder = os.path.basename(os.path.dirname(path))
        filename = os.path.basename(path)
        label = label_for(protocol_folder, filename)

        df = pd.read_csv(path)
        df = df[df["change_count"] != 0]
        df["Protocol"] = label
        df["Period"] = extract_period(filename) if label.startswith("RPBOB") else None
        frames.append(df)

    if not frames:
        raise FileNotFoundError(f"no matching files under {network_dir}")

    result = pd.concat(frames, ignore_index=True)

    if SELECTED_PROTOCOLS is not None:
        result = result[result["Protocol"].isin(SELECTED_PROTOCOLS)]

    if SELECTED_PERIODS is not None:
        is_rpbob = result["Protocol"].str.startswith("RPBOB")
        keep = (~is_rpbob) | result["Period"].isin(SELECTED_PERIODS)
        result = result[keep]

    if result.empty:
        raise FileNotFoundError(f"no rows left after filtering under {network_dir}")

    return result


def plot_ccdf(df, network_name):

    def display_label(row):
        if row["Protocol"].startswith("RPBOB") and pd.notna(row["Period"]):
            return rf"{row['Protocol']} ($\Delta={row['Period']:g}$)"
        return row["Protocol"]

    df = df.copy()
    df["Protocol"] = df.apply(display_label, axis=1)

    avg_times = df.groupby("Protocol")["stab_time"].mean()
    df["Protocol"] = df["Protocol"].map(lambda p: f"{p} (avg: {avg_times[p]:.2f} ms)")

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
    print(f"\tsaved {out_path}\n")


for network in discover_networks(DATA_ROOT):
    network_dir = os.path.join(DATA_ROOT, network)
    print(network_dir)
    try:
        df = load_metric(network_dir)
    except FileNotFoundError as e:
        print(f"skipping {network}: {e}")
        continue
    plot_ccdf(df, network)
