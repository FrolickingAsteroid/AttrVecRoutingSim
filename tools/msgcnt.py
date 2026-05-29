#!/usr/bin/env python3
import os
from asyncio.tasks import sleep
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


def make_ccdf(data):
    x = np.sort(np.asarray(data))
    n = len(x)

    if n == 0:
        return np.array([]), np.array([])

    ccdf = 1.0 - np.arange(0, n) / n

    x_plot = np.r_[0, x]
    ccdf_plot = np.r_[1.0, ccdf]

    return x_plot, ccdf_plot


def load_msg_counts_until_stab(csv_msg, csv_stab):
    msg = pd.read_csv(csv_msg)
    stab = pd.read_csv(csv_stab)

    # Last stabilization time per destination
    stab_time = stab.groupby("dest")["stab_time"].max()

    counts = []

    for dest, T in stab_time.items():
        mask = (
            (msg["dest"] == dest)
            & (msg["time"] <= T)
            & (msg["kind"].isin(["announcement", "propagate"]))
        )

        counts.append(msg.loc[mask, "msg_count"].sum())

    return np.sort(np.asarray(counts))


def load_wave_counts(csv_file, period):
    df = pd.read_csv(csv_file)

    announcements = df[df["kind"] == "announcement"].copy()
    propagations = df[df["kind"] == "propagate"].copy()

    wave_counts = []
    for _, ann_row in announcements.iterrows():
        t0 = ann_row["time"]

        if t0 > 50:
            continue

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


def plot_msgCnt_ccdf(series, network, output_file, period):

    plt.figure(figsize=(5.5, 3.8))
    max_x = 0
    for csv_msg, csv_stab, label in series:
        data = load_msg_counts_until_stab(csv_msg, csv_stab)

        x_plot, ccdf_plot = make_ccdf(data)
        avg = round(np.mean(data), 0)

        plt.step(
            x_plot,
            ccdf_plot,
            where="post",
            label=f"{label} (avg {avg:.0f})",
        )

        if np.max(x_plot) > max_x:
            max_x = np.max(x_plot)
    plt.title(f"Messages per Routing Computation (AS{network})")
    plt.xlabel("Number of Messages")
    plt.ylabel("CCDF")

    plt.xlim([0, max_x])
    plt.ylim([0, 1.1])
    plt.grid(True, alpha=0.3)

    plt.legend(fontsize=9)
    plt.tight_layout()

    plt.savefig(output_file, dpi=200, bbox_inches="tight")


if __name__ == "__main__":
    NETWORK = "1221"

    dprop = "dprop15.5"
    N = "N40"

    FILE_1 = f"DelayLength_all_{dprop}_p60.0_rc1_msgCount.csv"
    FILE_2 = f"DelayLength_all_{N}_p60.0_rc1_msgCount.csv"

    STAB_1 = f"DelayLength_all_{dprop}_p60.0_rc1_stab.csv"
    STAB_2 = f"DelayLength_all_{N}_p60.0_rc1_stab.csv"

    base_dir = Path(f"../data/output/rocketfuel/{NETWORK}")

    series = [
        (
            base_dir / "beaconing" / FILE_1,
            base_dir / "beaconing" / STAB_1,
            "Beaconing SP",
        ),
        (
            base_dir / "rpbob" / FILE_2,
            base_dir / "rpbob" / STAB_2,
            "RPBOB SP",
        ),
    ]

    plot_dir = Path("../data/plots")
    plot_dir.mkdir(exist_ok=True)

    output_file = plot_dir / f"AS{NETWORK}_Lengths_msgCount_{dprop}_{N}_ccdf_all.png"

    plot_msgCnt_ccdf(series, NETWORK, output_file, period=60)

    print(f"Output file: {output_file}")
