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

# BASE_DIR   : root of the output folder for this dataset
# DEST       : destination node, or "all"
# RUNS       : list of dicts, each describing one curve: ("rpbob", "beaconing", "bgp")

NETWORK = "1221"
STEM = "DelayLength"
BASE_DIR = Path(f"../data/output/rocketfuel/{NETWORK}")

RUNS = [
    {
        "proto": "rpbob",
        "params": {"": "lengths", "N": "40", "period": "60.0"},
        "label": "RPBOB SP N = 40",
    },
    {
        "proto": "bgp",
        "params": {"": "lengths"},
        "label": "BGP SP",
    },
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

DEST_FILTER = None  # e.g. 2 to plot only dest=2, None for all


# ─── helpers ─────────────────────────────────────────────────────────────────

def build_path(base_dir: Path, stem: str, proto: str, params: dict) -> Path:
    param_str = "_".join(f"{k}{v}" for k, v in params.items())
    filename = f"{stem}_{param_str}_convergence.csv"
    return base_dir / proto / filename


def build_series(base_dir: Path, stem: str, runs: list, dest_filter) -> list:
    """
    Returns a list of (DataFrame, label) pairs, one per run.
    Applies dest_filter if set.
    """
    series = []
    for run in runs:
        path = build_path(base_dir, stem, run["proto"], run["params"])
        if not path.exists():
            print(f"[warn] file not found: {path}")
            continue
        df = pd.read_csv(path)
        if dest_filter is not None:
            df = df[df["dest"] == dest_filter]
        series.append((df, run["label"]))
    return series


# ─── plot ─────────────────────────────────────────────────────────────────────

def plot_stab_ccdf(series: list, network: str, output_file: Path):
    plt.figure(figsize=(5.5, 3.8))
    max_x = 0

    for df, label in series:
        x = np.sort(df["stab_time"].to_numpy())
        n = len(x)
        avg_time = np.mean(x)
        ccdf = 1.0 - np.arange(0, n) / n

        x_plot = np.r_[0, x]
        ccdf_plot = np.r_[1.0, ccdf]

        plt.step(x_plot, ccdf_plot, where="post",
                 label=f"{label} (avg {avg_time:.2f} ms)")

    dest_str = f"dest={DEST_FILTER}" if DEST_FILTER is not None else "all destinations"
    plt.title(f"Stabilisation time per node (AS{network})")
    plt.xlabel("Time [ms]")
    plt.ylabel("CCDF")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)
    plt.xlim([0, max_x])
    plt.ylim([0, 1.1])

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
    output_file = plot_dir / f"AS{NETWORK}_{proto_tag}_{dest_tag}_stab_ccdf.png"

    plot_stab_ccdf(series, NETWORK, output_file)
