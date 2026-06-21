#!/usr/bin/env python3

from pathlib import Path
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
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

# BASE_DIR   : root of the output folder for this dataset
# DEST       : destination node, or "all"
# RUNS       : list of dicts, each describing one curve: ("rpbob", "beaconing", "bgp")

NETWORK = "1221"
FOLDER = "rocketfuel"
BASE_DIR = Path(f"../data/output/{FOLDER}/{NETWORK}")

ALGEBRA_LINESTYLES = {
    "lengths": "-",
    # "hops": "-",
}

PROTO_COLORS = {
    "bgp": "tab:green",
    "rpbob": "tab:blue",
}

RUNS = [
    # ─── normal ─────────────────────────────────────────────────────────────────
    # {
    #     "stem": "DelayLength",
    #     "proto": "bgp",
    #     "params": {"A_": "lengths", "S_": "normal"},
    #     "label": "BGP SP",
    # },
    # {
    #     "stem": "DelayLength",
    #     "proto": "rpbob",
    #     "params": {"A_": "lengths", "N": "0", "S_": "normal", "period": "0.0"},
    #     "label": "RPBOB SP",
    # },
    # {
    #     "stem": "DelayHops",
    #     "proto": "bgp",
    #     "params": {"A_": "hops", "S_": "normal"},
    #     "label": "BGP SP",
    # },
    # {
    #     "stem": "DelayHops",
    #     "proto": "rpbob",
    #     "params": {"A_": "hops", "N": "0", "S_": "normal", "period": "0.0"},
    #     "label": "RPBOB SP",
    # },
    # ─── linkFail ─────────────────────────────────────────────────────────────────
    # {
    #     "stem": "50_triangles",
    #     "proto": "bgp",
    #     "params": {"A_": "lengths", "S_": "linkFailure"},
    #     "label": "BGP SP",
    # },
    # {
    #     "stem": "50_triangles",
    #     "proto": "rpbob",
    #     "params": {"A_": "lengths", "N": "adaptative", "S_": "linkFailure", "period": "1.0"},
    #     "label": "RPBOB SP",
    # },
    # {
    #     "stem": "DelayLength",
    #     "proto": "bgp",
    #     "params": {"A_": "lengths", "S_": "linkFailure"},
    #     "label": "BGP SP",
    # },
    # {
    #     "stem": "DelayLength",
    #     "proto": "rpbob",
    #     "params": {"A_": "lengths", "N": "adaptative", "S_": "linkFailure", "period": "1.0"},
    #     "label": "RPBOB SP",
    # },
    # {
    #     "stem": "DelayLength",
    #     "proto": "rpbob",
    #     "params": {"A_": "lengths", "N": "adaptative", "S_": "linkFailure", "period": "16.0"},
    #     "label": "RPBOB SP",
    # },

    # {
    #     "stem": "DelayHops",
    #     "proto": "rpbob",
    #     "params": {"A_": "hops", "N": "adaptative", "S_": "linkFailure", "period": "1.0"},
    #     "label": "RPBOB H",
    # },
    # {
    #     "stem": "DelayHops",
    #     "proto": "bgp",
    #     "params": {"A_": "hops", "S_": "linkFailure"},
    #     "label": "BGP H",
    # },
    # {
    #     "proto":  "rpbob",
    #     "params": {"algebra": "lengths", "N": "40", "period": "60.0"},
    #     "label":  "RPBOB SP N=40",
    # },
    # {
    #     "proto":  "beaconing",
    #     "params": {"algebra": "lengths", "dprop": "15.5", "period": "60.0"},
    #     "label":  "Beaconing SP",
    # },
]

DEST_FILTER = None


# ─── helpers ─────────────────────────────────────────────────────────────────

def build_path(base_dir: Path, stem: str, proto: str, params: dict) -> Path:
    param_str = "_".join(f"{k}{v}" for k, v in params.items())
    filename = f"{stem}_{param_str}_convergence.csv"
    return base_dir / proto / filename


def build_series(base_dir: Path, runs: list, dest_filter) -> list:
    series = []
    for run in runs:
        path = build_path(base_dir, run["stem"], run["proto"], run["params"])
        if not path.exists():
            print(f"[warn] file not found: {path}")
            continue
        df = pd.read_csv(path)
        if dest_filter is not None:
            df = df[df["dest"] == dest_filter]
        series.append((df, run["label"], run["proto"], run["params"]["A_"]))
    return series


# ─── plot ─────────────────────────────────────────────────────────────────────

def plot_change_count(series: list, network: str, output_file: Path):
    fig, axes = plt.subplots(1, len(ALGEBRA_LINESTYLES), figsize=(5.5, 3.8), sharey=True)
    if len(ALGEBRA_LINESTYLES) == 1:
        axes = [axes]

    algebras = list(ALGEBRA_LINESTYLES.keys())
    tick_positions = np.array([], dtype=int)

    for ax, algebra in zip(axes, algebras):
        for i, proto in enumerate(PROTO_COLORS):
            match = [(df, label) for df, label, p, a in series if p == proto and a == algebra]
            if not match:
                continue
            df, _ = match[0]
            counts = df[df["change_count"] > 0]["change_count"]

            values, freqs = np.unique(counts, return_counts=True)
            freqs = freqs / freqs.sum()

            avg_change = np.average(values, weights=freqs)

            if len(tick_positions) == 0 or max(values) > max(tick_positions):
                tick_positions = np.union1d(tick_positions, values)

            offset = i * 0.4
            ax.bar(
                values + offset, freqs,
                color=PROTO_COLORS[proto],
                width=0.4,
                label=f"{proto.upper()} H (avg {avg_change:.2f} changes)",
            )

        ax.set_xlabel("Number of Attribute changes")
        ax.grid(True, axis="y", alpha=0.3)
        ax.xaxis.set_major_locator(plt.MaxNLocator(integer=True))

    axes[0].set_ylabel("Freq. per nodes")
    plt.title(f"Attribute changes per node after failure (AS{network})", fontsize=10)
    axes[-1].legend(fontsize=9)

    plt.setp(axes, xticks=tick_positions + 0.2, xticklabels=tick_positions)
    plt.xlim([-0.01, max(tick_positions) + 1])
    plt.ylim([-0.01, 1.00])

    plt.tight_layout()
    output_file.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_file, dpi=200, bbox_inches="tight")
    print(f"Saved: {output_file}")


def plot_stab_ccdf(series: list, network: str, output_file: Path):
    plt.figure(figsize=(5.5, 3.8))
    max_x = 0

    for df, label, proto, algebra in series:
        df = df[df["change_count"] != 0]

        x = np.sort(df["stab_time"].to_numpy())
        n = len(x)
        avg_time = np.mean(x)
        ccdf = 1.0 - np.arange(0, n) / n

        x_plot = np.r_[0, x]
        ccdf_plot = np.r_[1.0, ccdf]

        color = PROTO_COLORS.get(proto, None)

        plt.step(x_plot, ccdf_plot, where="post",
                 label=f"{label} (avg {avg_time:.2f} ms)", linewidth=1.5)

        if max_x < max(x_plot):
            max_x = max(x_plot)

    dest_str = f"dest={DEST_FILTER}" if DEST_FILTER is not None else "all destinations"
    plt.title(f"Stabilisation time per node  (AS{network})")
    plt.xlabel("Time [ms]")
    plt.ylabel("CCDF")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=9)
    plt.xlim([-0.01, max_x])
    plt.ylim([-0.01, 1.1])

    output_file.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_file, dpi=200, bbox_inches="tight")
    print(f"Saved: {output_file}")


# ─── main ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    series = build_series(BASE_DIR, RUNS, DEST_FILTER)

    if not series:
        print("No data to plot.")
        exit(1)

    plot_dir = Path("../data/plots")
    proto_tag = "_".join(dict.fromkeys(r["proto"] for r in RUNS))
    metric_tag = "_".join(dict.fromkeys(r["params"]["A_"] for r in RUNS))
    scene_tag = "_".join(dict.fromkeys(r["params"]["S_"] for r in RUNS))

    output_file = plot_dir / f"{NETWORK}_{proto_tag}_{scene_tag}_{metric_tag}_stab_ccdf.png"
    output_file_cc = plot_dir / f"{NETWORK}_{proto_tag}_{scene_tag}_{metric_tag}_change_count.png"

    plot_change_count(series, NETWORK, output_file_cc)
    plot_stab_ccdf(series, NETWORK, output_file)
