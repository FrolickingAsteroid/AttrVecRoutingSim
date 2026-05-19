#!/usr/bin/env python3

import math
import os
import re
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ATTR_RE = re.compile(r"\(\s*(-?\d+(?:\.\d+)?)\s*(?:[;,]\s*(-?\d+(?:\.\d+)?)\s*)?\)")


def parse_attr(s):
    m = ATTR_RE.search(str(s))
    if not m:
        raise ValueError(f"Could not parse attribute: {s}")

    first = float(m.group(1))
    second = m.group(2)

    if second is None:
        return (first,)

    return first, float(second)


def parse_set(s):
    s = str(s)

    pairs = []
    for m in ATTR_RE.finditer(s):
        first = float(m.group(1))
        second = m.group(2)

        if second is None:
            pairs.append((first,))
        else:
            pairs.append((first, float(second)))

    return pairs


def print_node_data(csv_file, node):
    df = pd.read_csv(csv_file)
    df_node = df[df["node"] == node].copy()

    if df_node.empty:
        raise ValueError(f"No data found for node {node}")

    df_nonempty_set = df_node[
        df_node["set"].notna()
        & (df_node["set"].astype(str).str.strip() != "")
        & (df_node["set"].astype(str).str.lower().str.strip() != "none")
    ]
    with pd.option_context("display.max_rows", None, "display.max_columns", None):
        print(df_nonempty_set)

    return df_node


def plot_timeseries(df_node, node, output_file=None):
    if df_node.empty:
        raise ValueError(f"No data found for node {node}")

    df_node = df_node.copy()
    df_node["attr"] = df_node["value"].apply(parse_attr)
    df_node["length"] = df_node["attr"].apply(lambda x: x[1])

    df_node = df_node.sort_values("time")

    plt.figure(figsize=(6.0, 3.6))
    plt.step(
        df_node["time"],
        df_node["length"],
        where="post",
        linewidth=1.6,
        marker="o",
        markersize=4,
    )

    plt.xlabel("Time")
    plt.ylabel("Length")
    plt.title(f"Node {node}")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=200, bbox_inches="tight")
        print(f"saved to {output_file}")


def plot_node_timeline(df_node, node, output_file=None, ncols=3):

    if df_node.empty:
        raise ValueError(f"No data found for node {node}")
    print(df_node)

    # Parse strings
    df_node["opt_pair"] = df_node["opt"].apply(parse_attr)
    df_node["set_pairs"] = df_node["set"].apply(parse_set)

    # Each row is one snapshot, even if time repeats
    snapshots = list(df_node.iterrows())
    nt = len(snapshots)

    # Compute global limits
    all_n = []
    all_a = []

    for _, row in df_node.iterrows():
        opt_n, opt_a = row["opt_pair"]
        all_n.append(opt_n)
        all_a.append(opt_a)

        for n, a in row["set_pairs"]:
            all_n.append(n)
            all_a.append(a)

    xmin = min(all_n) - 0.5
    xmax = max(all_n) + 0.5
    ymin = 0
    ymax = max(all_a) + 4

    nrows = math.ceil(nt / ncols)

    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(3.2 * ncols, 2.8 * nrows),
        squeeze=False,
    )

    # Each row is one snapshot, even if time repeats
    snapshots = list(df_node.iterrows())
    nt = len(snapshots)

    nrows = math.ceil(nt / ncols)

    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(3.2 * ncols, 2.8 * nrows),
        squeeze=False,
    )

    axes = axes.ravel()

    previous_opt = None

    for k, (ax, (_, row)) in enumerate(zip(axes, snapshots)):
        t = row["time"]

        opt_n, opt_a = row["opt_pair"]
        set_pairs = row["set_pairs"]

        # Plot all attributes in the set
        for n, a in set_pairs:
            if n == opt_n and a == opt_a:
                ax.plot([opt_n, opt_n], [0, opt_a], color="tab:blue", linewidth=1.2)
                ax.plot(opt_n, opt_a, marker="o", color="tab:blue", markersize=4)
            else:
                ax.plot([n, n], [0, a], color="black", linewidth=1.2)
                ax.plot(n, a, marker="o", color="black", markersize=4)

            ax.text(
                n,
                a + 0.6,
                str(a),
                ha="center",
                va="bottom",
                fontsize=9,
                color="black",
            )

        current_opt = (opt_n, opt_a)
        if previous_opt is None or current_opt != previous_opt:
            ax.set_facecolor("0.92")

        previous_opt = current_opt

        ax.set_title(rf"$T = {t}$", fontsize=10)

        ax.set_xlim(xmin, xmax)
        ax.set_ylim(ymin, ymax)

        # ax.spines["top"].set_visible(False)
        # ax.spines["right"].set_visible(False)
        # ax.spines["left"].set_visible(False)

        ax.set_xticks(sorted(set(all_n)))

    for ax in axes[nt:]:
        ax.axis("off")

    axes[0].set_xlabel("Version", fontsize=9)
    axes[0].set_ylabel("Length", fontsize=9)

    # fig.suptitle(f"Node {node}", fontsize=14)
    fig.tight_layout()

    if output_file:
        fig.savefig(output_file, dpi=200, bbox_inches="tight")
        print(f"saved to {output_file}")
    else:
        plt.show()


if __name__ == "__main__":
    PROTO = "beaconing"
    TESTDIR = f"../data/output/rocketfuel/3967/{PROTO}"
    FILE = "DelayLength_d21_p2.0_rc100_set.csv"
    node = 53

    cwd = os.getcwd()

    plot_dir = Path("../data/plots")
    plot_dir.mkdir(exist_ok=True)
    output_file = (
        plot_dir / f"{TESTDIR.split('/')[-1]}_{FILE.rsplit('.', 1)[0]}_node_{node}.png"
    )

    df_node = print_node_data(f"{TESTDIR}/{FILE}", node)
    plot_timeseries(df_node, node, output_file=output_file)
    # plot_node_timeline(df_node, node, output_file=output_file, ncols=3)
