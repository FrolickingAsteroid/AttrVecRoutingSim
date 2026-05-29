#!/usr/bin/env python3
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


def load_avg_messages_per_prop(csv_msg, csv_stab):
    msg = pd.read_csv(csv_msg)
    stab = pd.read_csv(csv_stab)

    # convergence time per destination
    stab_dest = (
        stab.groupby("dest", as_index=False)["stab_time"]
        .max()
        .rename(columns={"stab_time": "T_stab"})
    )

    prop = msg[msg["kind"] == "propagate"].copy()

    prop = prop.merge(stab_dest, on="dest", how="inner")
    prop = prop[prop["time"] <= prop["T_stab"]]

    # messages per destination and propagation time
    per_dest_time = prop.groupby(["dest", "time"], as_index=False)["msg_count"].sum()

    # average over destinations at each propagation time
    avg = (
        per_dest_time.groupby("time", as_index=False)["msg_count"]
        .mean()
        .sort_values("time")
    )

    return avg["time"].to_numpy(), avg["msg_count"].to_numpy()


def plot_avg_messages_per_prop(series, network, output_file):
    plt.figure(figsize=(5.5, 3.8))

    for csv_msg, csv_stab, label in series:
        x, y = load_avg_messages_per_prop(csv_msg, csv_stab)

        plt.step(
            x,
            y,
            where="post",
            label=label,
        )

    plt.title(f"Average Messages per Propagation Instance (AS{network})")
    plt.xlabel("Propagation instances [ms]")
    plt.ylabel("Average number of messages")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=9)
    plt.tight_layout()

    plt.savefig(output_file, dpi=200, bbox_inches="tight")


def plot_avg_msg_per_prop(series, network, output_file):
    plt.figure(figsize=(5.5, 3.8))

    for csv_msg, csv_stab, label in series:
        x, y = load_avg_messages_per_prop(csv_msg, csv_stab)

        bars = plt.bar(
            x,
            y,
            width=0.8,
            alpha=0.7,
            label=label,
            align="center",
        )

        for bar in bars:
            h = bar.get_height()
            plt.text(
                bar.get_x() + bar.get_width() / 2,
                h + 0.6,
                f"{h:.0f}",
                ha="center",
                va="bottom",
                fontsize=9,
                color="black",
            )

    plt.title(f"Average Messages per Propagation Instance (AS{network})")
    plt.xlabel("Propagation time [ms]")
    plt.ylabel("Average number of messages")
    plt.grid(True, alpha=0.3)
    plt.xticks(x)

    plt.ylim(0, y[-1] + 10)
    plt.legend(fontsize=9)
    plt.tight_layout()

    plt.savefig(output_file, dpi=200, bbox_inches="tight")


if __name__ == "__main__":
    NETWORK = "1221"

    dprop = "dprop5.0"
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
        # (
        #     base_dir / "rpbob" / FILE_2,
        #     base_dir / "rpbob" / STAB_2,
        #     "RPBOB SP",
        # ),
    ]

    plot_dir = Path("../data/plots")
    plot_dir.mkdir(exist_ok=True)

    output_file = plot_dir / f"AS{NETWORK}_Lengths_avgMsgPerProp_{dprop}_{N}.png"

    plot_avg_msg_per_prop(series, NETWORK, output_file)

    print(f"Output file: {output_file}")
