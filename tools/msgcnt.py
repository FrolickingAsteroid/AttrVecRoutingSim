#!/usr/bin/env python3
import os
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

plt.rcParams.update(
    {
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "font.size": 9,
    }
)


def make_ccdf(x):
    n = len(x)
    ccdf = 1.0 - np.arange(0, n) / n
    return np.r_[0, x], np.r_[1.0, ccdf]


def load_wave_counts(csv_file, period):
    df = pd.read_csv(csv_file)

    announcements = df[df["kind"] == "announcement"].copy()
    propagations = df[df["kind"] == "propagate"].copy()

    wave_counts = []
    for _, ann_row in announcements.iterrows():
        t0 = ann_row["time"]
        dest = ann_row["dest"]
        direct = ann_row["msg_count"]
        mask = (
            (propagations["dest"] == dest)
            & (propagations["time"] >= t0)
            & (propagations["time"] < t0 + period)
        )
        wave = propagations[mask]["msg_count"].sum()
        wave_counts.append(direct + wave)

    return np.sort(np.array(wave_counts))


def plot_msgCnt_ccdf(base_dir, file_name, network, output_dir, period):
    protos = ["beaconing", "rpbob"]
    colors = ["steelblue", "darkorange"]

    plt.figure(figsize=(5.5, 3.8))
    for proto, color in zip(protos, colors):
        csv_file = Path(base_dir) / proto / file_name

        data = load_wave_counts(csv_file, period)

        x_plot, ccdf_plot = make_ccdf(data)
        avg = round(np.mean(data), 0)

        plt.step(
            x_plot,
            ccdf_plot,
            where="post",
            color=color,
            label=f"{proto} (avg {avg:.0f})",
        )
    plt.title(f"Messages per Routing Computation (AS{network})")
    plt.xlabel("Number of Messages")
    plt.ylabel("CCDF")

    plt.ylim([0, 1.05])
    plt.grid(True, alpha=0.3)

    plt.legend(fontsize=9)
    plt.tight_layout()

    stem = Path(file_name).stem
    out = Path(output_dir) / f"{stem}_wave_ccdf.png"
    plt.savefig(out, dpi=200, bbox_inches="tight")
    print(f"Output file: {out}")


if __name__ == "__main__":
    BASE_DIR = "../data/output/rocketfuel/3967"
    FILE = "DelayLength_all_p3.0_rc10_msgCount.csv"

    plot_dir = Path("../data/plots")
    plot_dir.mkdir(exist_ok=True)

    AS = os.path.basename(Path(BASE_DIR))
    plot_msgCnt_ccdf(BASE_DIR, FILE, AS, plot_dir, period=3)
