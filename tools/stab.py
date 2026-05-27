#!/usr/bin/env python3

import csv
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
        "font.size": 10,
        # "axes.labelsize": 10,
        # "axes.titlesize": 12,
        # "xtick.labelsize": 8,
        # "ytick.labelsize": 8,
        # "legend.fontsize": 10,
    }
)


def plot_stab_ccdf(series, network, output_file=None):
    plt.figure(figsize=(5.5, 3.8))
    max_x = 0

    for csv_file, label in series:
        print(f"{csv_file}")
        df = pd.read_csv(csv_file)

        x = np.sort(df["stab_time"].to_numpy())
        n = len(x)

        avg_time = np.mean(x)
        ccdf = 1.0 - np.arange(0, n) / n

        x_plot = np.r_[0, x]
        ccdf_plot = np.r_[1.0, ccdf]

        plt.step(x_plot, ccdf_plot, where="post", label=f"{label} ({avg_time:.2f} ms)")
        if np.max(x) > max_x:
            max_x = np.max(x)
            print(f"\tMaximum delay: {max_x}\n")

    plt.title(f"Stabilisation time per node (AS{network})")
    plt.xlabel("Time [ms]")
    plt.ylabel("CCDF")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)

    # plt.xlim([0, max_x])
    plt.ylim([0, 1.1])
    plt.savefig(output_file, dpi=200, bbox_inches="tight")


if __name__ == "__main__":
    NETWORK = "1221"

    FILE_1 = "DelayLength_all_p60.0_rc1_stab.csv"
    # FILE_2 = "DelayHops_all_p60.0_rc1_stab.csv"

    base_dir = Path(f"../data/output/rocketfuel/{NETWORK}")

    series = [
        (base_dir / "rpbob" / FILE_1, "RPBOB SP"),
        (base_dir / "beaconing" / FILE_1, "Beaconing SP"),
        # (base_dir / "rpbob" / FILE_2, "RPBOB Hop Count"),
        # (base_dir / "beaconing" / FILE_2, "Beaconing Hop Count"),
    ]

    plot_dir = Path("../data/plots")
    plot_dir.mkdir(exist_ok=True)

    output_file = plot_dir / f"AS{NETWORK}_Lengths_ccdf_all.png"

    plot_stab_ccdf(series, NETWORK, output_file)

    print(f"Output file: {output_file}")
