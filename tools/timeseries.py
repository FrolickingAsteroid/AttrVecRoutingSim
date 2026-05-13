#!/usr/bin/env python3

import math
import re

import matplotlib.pyplot as plt
import pandas as pd

PAIR_RE = re.compile(r"\((-?\d+)\s*[;,]\s*(-?\d+)\)")


def parse_attr(s):
    """
    Parses '(n;a)' or '(n,a)' into (n, a).
    """
    m = PAIR_RE.search(str(s))
    if not m:
        raise ValueError(f"Could not parse attribute: {s}")

    return int(m.group(1)), int(m.group(2))


def parse_set(s):
    """
    Parses '[(0;1) (1;1) (2;3)]' into [(0,1), (1,1), (2,3)].
    """
    s = str(s)

    return [(int(n), int(a)) for n, a in PAIR_RE.findall(s)]


def plot_node_timeline(csv_file, node, output_file=None, ncols=3):
    df = pd.read_csv(csv_file)

    df_node = df[df["node"] == node].copy()

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
    csv_file = "/home/novak/Documents/Work/Thesis/Sim/data/output/tests/small/DelayLengths-one-neigh-5-2.csv"
    node = 1
    output_file = f"node_{node}.png"

    plot_node_timeline(csv_file, node, output_file=output_file, ncols=3)
