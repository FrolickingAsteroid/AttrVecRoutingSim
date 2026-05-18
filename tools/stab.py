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
        "font.size": 10,
        # "axes.labelsize": 10,
        # "axes.titlesize": 12,
        # "xtick.labelsize": 8,
        # "ytick.labelsize": 8,
        # "legend.fontsize": 10,
    }
)


def plot_stab_ccdf(csv_file, network, output_file=None):
    df = pd.read_csv(csv_file)
    x = np.sort(df["stab_time"].to_numpy())
    n = len(x)

    avg_time = np.mean(x)

    ccdf = 1.0 - np.arange(0, n) / n

    # prepend starting point
    x_plot = np.r_[0, x]
    ccdf_plot = np.r_[1.0, ccdf]

    plt.figure(figsize=(5.5, 3.8))
    plt.step(x_plot, ccdf_plot, where="post", label=f"RPBOB SP ({avg_time:.2f} ms)")

    plt.title(f"Stabilisation time per node (AS{network})")
    plt.xlabel("Time [ms]")
    plt.ylabel("CCDF")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)

    ax = plt.gca()
    ax.set_xlim([0, max(x)])

    if output_file:
        plt.savefig(output_file, dpi=200, bbox_inches="tight")
    else:
        plt.show()


if __name__ == "__main__":
    TESTDIR = "../data/output/rocketfuel/3967"
    FILE = "DelayLength_all_p1.0_rc3_stab.csv"

    cwd = os.getcwd()

    plot_dir = Path(cwd) / "plots"
    plot_dir.mkdir(exist_ok=True)
    output_file = (
        plot_dir / f"{TESTDIR.split('/')[-1]}_{FILE.rsplit('.', 1)[0]}_ccdf.png"
    )

    plot_stab_ccdf(f"{TESTDIR}/{FILE}", Path(TESTDIR).stem, output_file)
    print(f"Output file: {output_file}")
