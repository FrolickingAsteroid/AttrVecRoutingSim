#!/usr/bin/env python3

import matplotlib.pyplot as plt
from pathlib import Path
import seaborn as sns
import pandas as pd
from matplotlib.lines import Line2D
import itertools
import numpy as np
import argparse
import re

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 11,
    "axes.labelsize": 11,
    "xtick.labelsize": 11,
    "ytick.labelsize": 11,
    "legend.fontsize": 9,
})

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)


DATASET = "rocketfuel"
NETWORK = "1755"
SCENARIO = "normal"
ALGEBRAS = ["Hops", "SP"]
PERIODS = ["1.0"]
SEQS = []
NOTIF = False
XMAX = None
FILES_TO_PLOT = ["convergence", "msglog"]   # one column each, side by side, sharing the CCDF axis

MAX_STAB = {"Hops": 69.4476, "SP": 62.7138}
MAX_MSG= {"Hops": 69.4476, "SP": 62.7138}

COLUMNWIDTH = 345.0     # LaTeX \columnwidth in pt
PANEL_WF = 1.
PANEL_HF =0.9
WSPACE = 0.04           # gap between columns, as a fraction of a panel's width
PALETTE = "Set2"        # colours: the first column takes the first ones, the next column the following ones
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-explicit": "All SP",
    "lex-hops-explicit": "All Hops",
    "lex-length-hops-suffix": "SP",
    # "lex-length-width-suffix": "WSP",
    # "prod-width-length-hops-suffix": "Product",
}

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

def read_stab_times(csv_path):
    df = pd.read_csv(csv_path)
    if "change_count" in df:
        df = df[df["change_count"] != 0]
    return df.loc[df["stab_time"] != 0, "stab_time"].to_numpy()


def read_msgs_per_dest(csv_path):
    df = pd.read_csv(csv_path)
    return df.groupby("dest")["msg"].sum().to_numpy()

FILES = {

    "convergence": dict(read=read_stab_times, xlabel="Time [ms]",
                        avg=lambda v: f"{v:>4.2f} ms",
                        marks=MAX_STAB, marks_label="Max Node Stabilization Time:",
                        time=True, name="ccdf_seq"),
    "msglog": dict(read=read_msgs_per_dest, xlabel="Number of Messages",
                   avg=lambda v: f"{round(v):>4d} msgs",
                   marks=MAX_MSG, marks_label="",
                   time=False, name="ccdf_msgs_seq"),
}



parser = argparse.ArgumentParser(description="RRP CCDFs, one row per version (sequence number).")
parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
parser.add_argument("-n", "--network", default=NETWORK, help=f"default: {NETWORK}")
parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
parser.add_argument("-a", "--algebras", nargs="+", default=ALGEBRAS, help=f"default: {' '.join(ALGEBRAS)}")
parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"one figure per period (default: {' '.join(PERIODS)})")
parser.add_argument("--seqs", nargs="+", type=int, default=SEQS, help="versions to show (default: all)")
parser.add_argument("--notif", action="store_true", default=NOTIF, help="use the ..._notif_... files")
parser.add_argument("--xmax", type=float, default=XMAX, help="right edge of the x axis (default: largest value)")
parser.add_argument("--files", nargs="+", choices=list(FILES), default=FILES_TO_PLOT,
                    help=f"one column per file, left to right (default: {' '.join(FILES_TO_PLOT)})")
args = parser.parse_args()

FILES_TO_PLOT, DATASET, NETWORK, SCENARIO, ALGEBRAS = args.files, args.topo, args.network, args.scenario, args.algebras
PERIODS, SEQS, NOTIF, XMAX = args.periods, args.seqs, args.notif, args.xmax


def ecdf(data, start=0.0):
    n = len(data)
    x = np.sort(data)
    y = 1 - np.arange(1, n + 1) / n
    return np.concatenate([[min(start, x[0])], x]), np.concatenate([[1.0], y])


def load(period, file):
    folder = ROOT_DIR / 'data' / 'output' / DATASET / NETWORK / "rpbob"
    curves = {Algebra.get(a, a): {} for a in ALGEBRAS}
    for csv_path in folder.glob(f"*_S_{SCENARIO}_*period_{period}_{file}_seq_*.csv"):
        filename = csv_path.name

        if ("_notif_" in filename) != NOTIF:
            continue

        m = re.search(r"_A_(.+?)_[NS]_", filename)
        algebra = Algebra.get(m.group(1), m.group(1)) if m else ""
        if algebra not in curves:
            continue

        seq = int(re.search(r"_seq_(\d+)\.csv$", filename).group(1))
        if SEQS and seq not in SEQS:
            continue

        print(csv_path)
        curves[algebra][seq] = FILES[file]["read"](csv_path)

    curves = {a: dict(sorted(c.items())) for a, c in curves.items() if c}


    if not curves:
        raise SystemExit(f"Nothing found in {folder} for {file}, period {period}, algebras {ALGEBRAS}.")
    return curves

def draw_row(ax, s, curves, shift, colors, pad, metric):
    handles, labels = [], []
    for a, c in curves.items():
        if s not in c:
            continue
        x, y = ecdf(c[s] + shift, start=shift)
        line, = ax.step(x, y, where="post", color=colors[a], lw=2)
        handles.append(line)
        labels.append(a.ljust(pad) + f"({metric['avg'](c[s].mean())})")

    legend = ax.legend(handles, labels, loc="upper right", fancybox=True, shadow=False, framealpha=1)
    for text in legend.get_texts():
        text.set_fontfamily("monospace")


def style_row(ax, i, n, s, xmax, first_col=True, last_col=True):
    if first_col:
        ax.text(-0.015, 0.4, f"{s}", transform=ax.transAxes, ha="right", va="center")
    ax.set_ylim(-0.03, 1.05)
    ax.set_yticks([0, 0.5, 1])
    ax.yaxis.tick_right()
    ax.tick_params(axis="y", length=2.5, pad=2)
    ax.tick_params(axis="x", length=0 if i < n - 1 else 3.5)

    if last_col:
        ax.spines["right"].set_bounds(0, 1)
        ax.tick_params(axis="y", labelright=True)
    else:
        ax.spines["right"].set_visible(False)
        ax.tick_params(axis="y", right=False, labelright=False)
    ax.spines["bottom"].set_visible(i == n - 1)
    ax.spines["top"].set_visible(False)
    ax.spines["left"].set_visible(True)
    ax.grid(axis="x", alpha=0.7)
    ax.set_xlim(0, xmax)


def delta_ticks(ax, delta, xmax, max_ticks=10):
    n = xmax / delta
    step = 10
    step = next((k for k in (1, 2, 5, 10, 20, 50, 100, 200, 500) if n / k <= max_ticks), int(np.ceil(n / max_ticks)))
    ks = np.arange(0, n + step, step).astype(int)
    ax.set_xticks(ks * delta, ["0" if k == 0 else r"Δ" if k == 1 else rf"{k} Δ" for k in ks])


def mark_max(ax, colors, metric):
    if not metric["marks_label"]:
        return
    shown = {a: xv for a, xv in metric["marks"].items() if a in colors}
    for a, xv in shown.items():
        ax.plot(xv, 0, "o", c=colors[a], clip_on=False, markersize=5)

    leg = ax.figure.legend(
        handles=[Line2D([], [], linestyle="none")]
                + [Line2D([], [], marker="o", markersize=5, linestyle="none", color=colors[a]) for a in shown],
        labels=[metric["marks_label"]] + list(shown),
        loc="lower center", bbox_to_anchor=(0.5, 1), bbox_transform=ax.transAxes,
        ncol=len(shown) + 1, frameon=False, handlelength=1, handletextpad=0.3, columnspacing=1.0,
    )
    for text in leg.get_texts():
        text.set_fontfamily("monospace")


def plot_ridge(period):
    """One figure: a column per file in FILES_TO_PLOT, a row per version, all sharing the CCDF axis."""
    data = {f: load(period, f) for f in FILES_TO_PLOT}      # file -> algebra -> {version: values}
    delta = float(period)
    algebras = list(dict.fromkeys(a for curves in data.values() for a in curves))
    seqs = sorted({s for curves in data.values() for c in curves.values() for s in c})
    first = seqs[0]
    # each column gets its own colours: column 0 takes the first colours of the palette, column 1 the next, ...
    palette = sns.color_palette(PALETTE, n_colors=len(algebras) * len(FILES_TO_PLOT))
    colors_of = {f: dict(zip(algebras, palette[j * len(algebras):(j + 1) * len(algebras)]))
                 for j, f in enumerate(FILES_TO_PLOT)}

    # Not needed, perhaps ask
    # linestyles = dict(zip(algebras, itertools.cycle(['-', 'dotted'])))

    pad = max(len(a) for a in algebras) + 1

    def shift_of(s):
        return (s - first) * delta

    n, m = len(seqs), len(FILES_TO_PLOT)
    hspace = 0.2
    fig, axes = plt.subplots(n, m, sharex="col", sharey=True, squeeze=False,
                             figsize=get_figsize(COLUMNWIDTH, wf=PANEL_WF * m, hf=PANEL_HF / m),
                             gridspec_kw={"hspace": hspace, "wspace": WSPACE})

    for j, file in enumerate(FILES_TO_PLOT):
        metric, curves, colors = FILES[file], data[file], colors_of[file]
        xmax = XMAX or max(d.max() + shift_of(s) for c in curves.values() for s, d in c.items())

        for i, s in enumerate(seqs):
            ax = axes[i, j]
            draw_row(ax, s, curves, shift_of(s), colors, pad, metric)
            style_row(ax, i, n, s, xmax, first_col=(j == 0), last_col=(j == m - 1))
            if metric["time"]:
                delta_ticks(ax, delta, xmax)
                ax.set_xlim(0, xmax-1)

        axes[-1, j].set_xlabel(metric["xlabel"])
        #mark_max(axes[0, j], colors, metric)


    right = axes[0, -1]
    left = axes[0, 0]
    right.yaxis.set_label_position("right")
    right.set_ylabel("CCDF")
    left.set_ylabel("Routing Computation Instance")
    right.yaxis.set_label_coords(1.1, 1 - (n + (n - 1) * hspace) / 2)
    left.yaxis.set_label_coords(-0.08, 1 - (n + (n - 1) * hspace) / 2)

    notif = "notif_" if NOTIF else ""
    prefix = FILES[FILES_TO_PLOT[0]]["name"] if m == 1 else "ccdf_seq_" + "_".join(FILES_TO_PLOT)
    name = f"{prefix}_{NETWORK}_{SCENARIO}_{'_'.join(a.replace(' ', '') for a in algebras)}_{notif}period_{period}.pdf"
    out_path = OUTPUT_DIR / name
    plt.savefig(out_path, format="pdf", bbox_inches="tight")
    plt.show()
    print(f"\tsaved {out_path}\n")


if __name__ == "__main__":
    for period in PERIODS:
        plot_ridge(period)