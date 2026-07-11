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
        "font.size": 10,
    }
)

NETWORK = "1221"
STEM = "DelayLength"
BASE_DIR = Path(f"../../data/output/rocketfuel/{NETWORK}")

CONV_SUFFIX = "convergence"
MSG_SUFFIX = "msglog"

RUNS = [
    {
        "proto": "rpbob",
        "params": {
            "A_": "lengths",
            "N": "adaptative",
            "S_": "linkFailure",
            "period": "1.0",
        },
        "label": "RPBOB SP",
    },
    {
        "proto": "bgp",
        "params": {"A_": "lengths", "S_": "linkFailure"},
        "label": "BGP SP",
    },
]

DEST_FILTER = None
BIN_WIDTH = 1.0


# ─── helpers ──────────────────────────────────────────────────────────────────


def build_path(
    base_dir: Path, stem: str, proto: str, params: dict, suffix: str
) -> Path:
    param_str = "_".join(f"{k}{v}" for k, v in params.items())
    filename = f"{stem}_{param_str}_{suffix}.csv"
    return base_dir / proto / filename


def build_series(base_dir: Path, stem: str, runs: list, dest_filter) -> list:
    series = []

    for run in runs:
        conv_path = build_path(base_dir, stem, run["proto"], run["params"], CONV_SUFFIX)
        msg_path = build_path(base_dir, stem, run["proto"], run["params"], MSG_SUFFIX)

        if not conv_path.exists():
            print(f"[warn] file not found: {conv_path}")
            continue

        if not msg_path.exists():
            print(f"[warn] file not found: {msg_path}")
            continue

        conv = pd.read_csv(conv_path)
        msg = pd.read_csv(msg_path)

        if dest_filter is not None:
            conv = conv[conv["dest"] == dest_filter]
            msg = msg[msg["dest"] == dest_filter]

        conv = conv[conv["change_count"] != 0]

        series.append((conv, msg, run["label"]))

    return series


def msg_rate_until_convergence(conv: pd.DataFrame, msg: pd.DataFrame, bin_width: float):
    conv_time = (
        conv.groupby(["failure", "dest"])["stab_time"]
        .max()
        .reset_index()
        .rename(columns={"stab_time": "T_conv"})
    )

    df = msg.merge(conv_time, on=["failure", "dest"], how="inner")

    # If your message times are already measured from the failure instant, keep this.
    # Otherwise subtract the failure time here.
    df["t_rel"] = df["time"]

    df = df[(df["t_rel"] >= 0) & (df["time"] <= df["T_conv"])]

    df["bin"] = np.floor(df["t_rel"] / bin_width).astype(int)
    df["t_bin"] = df["bin"] * bin_width

    # Messages per failure/destination/time-bin
    per_exp = df.groupby(["failure", "dest", "t_bin"])["msg"].sum().reset_index()

    # Average profile over all failure/destination experiments
    avg = (
        per_exp.groupby("t_bin")["msg"]
        .agg(
            mean="mean",
            median="median",
            q25=lambda x: np.percentile(x, 25),
            q75=lambda x: np.percentile(x, 75),
        )
        .reset_index()
    )

    return avg


# ─── plots ────────────────────────────────────────────────────────────────────


def plot_msg_rate(series: list, network: str, output_file: Path):
    plt.figure(figsize=(5.5, 3.8))
    max_x = 0

    for conv, msg, label in series:
        avg = msg_rate_until_convergence(conv, msg, BIN_WIDTH)

        if avg.empty:
            continue

        plt.plot(
            avg["t_bin"],
            avg["mean"],
            linewidth=1.5,
            label=label,
        )

        plt.fill_between(
            avg["t_bin"],
            avg["q25"],
            avg["q75"],
            alpha=0.18,
        )

        max_x = max(max_x, avg["t_bin"].max())

    plt.title(f"Message rate after failure (AS{network})")
    plt.xlabel("Time since failure [ms]")
    plt.ylabel(f"Messages per {BIN_WIDTH:g} ms")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)
    plt.xlim([0, max_x])

    output_file.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_file, dpi=200, bbox_inches="tight")
    print(f"Saved: {output_file}")


def plot_cumulative_msgs(series: list, network: str, output_file: Path):
    plt.figure(figsize=(5.5, 3.8))
    max_x = 0

    for conv, msg, label in series:
        avg = msg_rate_until_convergence(conv, msg, BIN_WIDTH)

        if avg.empty:
            continue

        avg["cum_mean"] = avg["mean"].cumsum()

        plt.step(
            avg["t_bin"],
            avg["cum_mean"],
            where="post",
            linewidth=1.5,
            label=label,
        )

        max_x = max(max_x, avg["t_bin"].max())

    plt.title(f"Cumulative messages after failure (AS{network})")
    plt.xlabel("Time since failure [ms]")
    plt.ylabel("Cumulative messages")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)
    plt.xlim([0, max_x])

    output_file.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_file, dpi=200, bbox_inches="tight")
    print(f"Saved: {output_file}")


# ─── main ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    series = build_series(BASE_DIR, STEM, RUNS, DEST_FILTER)

    if not series:
        print("No data to plot.")
        exit(1)

    plot_dir = Path("../data/plots")
    proto_tag = "_".join(r["proto"] for r in RUNS)
    dest_tag = f"d{DEST_FILTER}" if DEST_FILTER is not None else "all"

    msg_rate_file = plot_dir / f"AS{NETWORK}_{proto_tag}_{dest_tag}_msg_rate.png"
    cum_msg_file = plot_dir / f"AS{NETWORK}_{proto_tag}_{dest_tag}_cum_msg.png"

    plot_msg_rate(series, NETWORK, msg_rate_file)
