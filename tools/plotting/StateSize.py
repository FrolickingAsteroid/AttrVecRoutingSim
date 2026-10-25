#!/usr/bin/env python3
import matplotlib.pyplot as plt
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import argparse
import re
import os

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 11,
    "axes.labelsize": 11,
    "xtick.labelsize": 11,
    "ytick.labelsize": 11,
    "legend.fontsize": 9,
})


# Source - https://stackoverflow.com/a/31527287
# Posted by rth, modified by community. See post 'Timeline' for change history
# Retrieved 2026-09-27, License - CC BY-SA 4.0

def get_figsize(columnwidth, wf=0.5, hf=(5. ** 0.5 - 1.0) / 2.0, ):
    """Parameters:
      - wf [float]:  width fraction in columnwidth units
      - hf [float]:  height fraction in columnwidth units.
                     Set by default to golden ratio.
      - columnwidth [float]: width of the column in latex. Get this from LaTeX
                             using \\showthe\\columnwidth
    Returns:  [fig_width,fig_height]: that should be given to matplotlib
    """
    fig_width_pt = columnwidth * wf
    inches_per_pt = 1.0 / 72.27  # Convert pt to inch
    fig_width = fig_width_pt * inches_per_pt  # width in inches
    fig_height = fig_width * hf  # height in inches
    return [fig_width, fig_height]


ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "rocketfuel"
NETWORKS = ["1755"]
SCENARIO = "normal"
Proto_label = ["RRP", "BGP", "BGP-RCN"]
Algebra_label = ["SP"]
PERIODS = ["1.0"]
LOGX = False

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

# state.csv (bgp / rcn) and the window csv (rrp) live in different protocol directories
PROTOCOL = {
    "window": "RRP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
}

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-explicit": "All SP",
    "lex-hops-explicit": "All Hops",
    "lex-length-hops-suffix": "SP",
    "lex-length-width-suffix": "WSP",
    "prod-width-length-hops-suffix": "Product",
}


def get_algebra(filename: str) -> str:
    m = re.search(r"_A_(.+?)_S_", filename)
    return Algebra.get(m.group(1), m.group(1)) if m else ""


def get_period(filename: str) -> str:
    m = re.search(r"period_(\d+\.\d+)", filename)
    return m.group(1) if m else ""


def load(network):
    """One row per (failure,) destination and node: the number of state entries the node keeps for that destination."""
    rows = []
    net_dir = ROOT_DIR / 'data' / 'output' / DATASET / network

    for protocol, label in PROTOCOL.items():
        if label not in Proto_label:
            continue

        pattern = f"*_S_{SCENARIO}_*.csv" if protocol == "window" else f"*_S_{SCENARIO}_state.csv"
        for csv_path in (net_dir / protocol).rglob(pattern):
            filename = csv_path.name
            algebra = get_algebra(filename)
            if algebra not in Algebra_label:
                continue

            df = pd.read_csv(csv_path)
            df = df[df["node"] != df["dest"]]  # the destination keeps no state towards itself

            if protocol == "window":
                period = get_period(filename)
                if period not in PERIODS:
                    continue
                df = df[df["opt_time"].notna()]  # unreachable nodes hold no window
                parts = {rf"RRP $\Delta = {period}$ ms" if len(PERIODS) > 1 else "RRP": df["wind_size"]}
            else:
                parts = {label: df["rib_size"] + df["seq_size"]}  # seq_size is 0 for bgp

            print(csv_path)
            for name, values in parts.items():
                rows.append(pd.DataFrame({"value": values.to_numpy(), "Protocol": name, "Algebra": algebra}))

    if not rows:
        raise SystemExit(f"Nothing found for {DATASET}/{network}, scenario '{SCENARIO}', protocols {Proto_label}, "
                         f"algebras {Algebra_label}, periods {PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def ccdf(data):
    n = len(data)
    x = np.sort(data)
    y = 1 - np.arange(1, n + 1) / n

    x = np.concatenate([[x[0] if LOGX else 0.0], x])  # 0 can't be shown on a log axis
    y = np.concatenate([[1.0], y])
    return x, y


def plot_ccdf(df, network_name):
    groups = list(df.groupby("Protocol", sort=False))
    protocols = sorted(p for p, _ in groups)
    colors = dict(zip(protocols, sns.color_palette(palette="colorblind", n_colors=len(protocols))))

    pad_width = max(len(p.replace(r"\Delta", "Delta").replace("$", "")) for p, _ in groups) + 2

    columnwidth = 345.0
    fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1, hf=0.8))

    for protocol, group in groups:
        x, y = ccdf(group["value"].to_numpy())
        ax.plot(
            x, y,
            color=colors[protocol],
            drawstyle="steps-post",
            alpha=0.8,
            linewidth=2,
            label=protocol.ljust(pad_width) + f"(avg {group['value'].mean():>4.1f})",
        )

    ax.set_ylim([-1e-2, 1.1])
    if LOGX:
        ax.set_xscale("log")
        ax.set_xlim([df["value"].min(), df["value"].max()])
    else:
        ax.set_xlim([0.0, df["value"].max() + 1])
        ax.xaxis.get_major_locator().set_params(integer=True)  # entries are counts
    ax.set_xlabel("State entries per node and destination")
    ax.set_ylabel("CCDF")
    ax.grid(alpha=0.3, which="both")

    legend = ax.legend(loc='upper right', fancybox=True, shadow=False)
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    out_path = os.path.join(OUTPUT_DIR, f"state_{network_name}_{SCENARIO}.pdf")
    plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
    plt.show()
    print(f"\tsaved {out_path}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot CCDFs of the routing state kept per node and destination.")
    parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
    parser.add_argument("-n", "--networks", nargs="+", default=NETWORKS, help=f"default: {' '.join(NETWORKS)}")
    parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
    parser.add_argument("-p", "--protocols", nargs="+", default=Proto_label, help=f"default: {' '.join(Proto_label)}")
    parser.add_argument("-a", "--algebras", nargs="+", default=Algebra_label,
                        help=f"default: {' '.join(Algebra_label)}")
    parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"RRP periods (default: {' '.join(PERIODS)})")
    parser.add_argument("--logx", action="store_true", default=LOGX, help="log x axis")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    LOGX = args.logx

    for network in NETWORKS:
        df = load(network)
        plot_ccdf(df, network)
