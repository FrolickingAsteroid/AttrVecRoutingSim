#!/usr/bin/env python3

import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np
import seaborn as sns
import pandas as pd
import argparse
import re


plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 10,
    "axes.labelsize": 12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 10,
})

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "clique"
NETWORKS = ["64"]
SCENARIO = "linkFailure"
Proto_label = ["SyRuP", "SyRuP-LFN", "BGP", "BGP-RCN"]
PERIODS = ["1.0"]
BCN_PERIODS = ["31.0"]
DPROPS = ["5.0"]
ALGEBRAS = []
BIN = 20
XMIN = 100
XMAX = None
PAD = True


LINEWIDTH = 345.0

SIDE_BY_SIDE = True
WF, HF = (1, 0.85) if SIDE_BY_SIDE else (1.0, (5. ** 0.5 - 1.0) / 2.0)


T_FAIL = 100
STEADY_FROM = 50
EXTRAPOLATE_SYRUP = True

NETWORK_DIR = {
    "clique": "bclique_n{}",
    "triangle": "triangle_n{}",
    "rocketfuel": "{}",
    "scion": "{}",
}

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "rpbob": "SyRuP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
    "beaconing": "Beaconing",
}

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-explicit": "All SP",
    "lex-hops-explicit": "All Hops",
    "lex-length-hops-suffix": "SP",
    "lex-length-width-suffix": "WSP",
    "prod-width-length-hops-suffix": "Product",
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


MARKERS = ['s', "o", "v", "D", "^", "P", "X", "*"]

parser = argparse.ArgumentParser(description="Plot the rate of messages exchanged over time.")
parser.add_argument("-t", "--topo", default=DATASET, choices=NETWORK_DIR, help=f"default: {DATASET}")
parser.add_argument("-n", "--networks", nargs="+", default=NETWORKS, help=f"default: {' '.join(NETWORKS)}")
parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
parser.add_argument("-p", "--protocols", nargs="+", default=Proto_label, help=f"default: {' '.join(Proto_label)}")
parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"default: {' '.join(PERIODS)}")
parser.add_argument("--bperiods", nargs="+", default=BCN_PERIODS,
                    help=f"beaconing announcement periods (default: {' '.join(BCN_PERIODS)})")
parser.add_argument("--dprops", nargs="+", default=DPROPS,
                    help=f"beaconing propagation intervals (default: {' '.join(DPROPS)})")
parser.add_argument("-a", "--algebras", nargs="+", default=ALGEBRAS,
                    help="algebras to plot, e.g. Hops SP (or lex-hops-suffix); default: all")
parser.add_argument("--bin", type=int, default=BIN, help=f"bin width (default: {BIN})")
parser.add_argument("--xmin", type=float, default=XMIN, help=f"left edge of the x axis (default: {XMIN})")
parser.add_argument("--xmax", type=float, default=XMAX,
                    help="right edge of the x axis, up to which SyRuP's pattern is continued (default: end of the logs)")
parser.add_argument("--no-pad", dest="pad", action="store_false", default=PAD,
                    help="don't pad lines that end early with zeros")
parser.add_argument("--save", action="store_true", help="also save the plot to data/plots")
parser.add_argument("--linewidth", type=float, default=LINEWIDTH,
                    help=rf"width of the line in the paper, in pt (\showthe\linewidth; default: {LINEWIDTH})")
parser.add_argument("--wf", type=float, default=WF, help=f"width as a fraction of --linewidth (default: {WF})")
parser.add_argument("--hf", type=float, default=HF, help=f"height as a fraction of the width (default: {HF})")
args = parser.parse_args()

DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
Proto_label, PERIODS, ALGEBRAS = args.protocols, args.periods, args.algebras
BCN_PERIODS, DPROPS = args.bperiods, args.dprops
BIN, XMIN, XMAX, PAD = args.bin, args.xmin, args.xmax, args.pad
LINEWIDTH, WF, HF = args.linewidth, args.wf, args.hf
DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob" and "notif" in filename.lower():
        return "SyRuP-LFN"
    return PROTOCOL.get(key, protocol_raw.upper())



def bin_series(df, bin_freq='10s', bin=10):
    df = df.copy()
    df['time_ms'] = pd.to_timedelta(df['time'], unit='s')
    series = df.groupby('time_ms')['msg'].sum()
    binned = series.resample(bin_freq).sum()

    binned.index = binned.index.total_seconds().astype(int)
    return binned


rows = []
raw_frames = {}
sort_keys = {}

for n in NETWORKS:
    net_dir = DATA_DIR / NETWORK_DIR[DATASET].format(n)
    for csv_path in net_dir.glob(f"*/{SCENARIO}/*_S_{SCENARIO}_*msglog.csv"):
        protocol = csv_path.parent.parent.name
        filename = csv_path.name
        protocol_label = get_protocol_label(protocol, csv_path.name)

        if protocol_label not in Proto_label:
            print(protocol_label)
            print(csv_path)
            continue

        m = re.search(r"_A_(.+?)_[NS]_", filename)
        algebra_raw = m.group(1) if m else ""
        algebra = Algebra.get(algebra_raw, algebra_raw)
        if ALGEBRAS and algebra not in ALGEBRAS and algebra_raw not in ALGEBRAS:
            continue

        base = protocol_label
        delta = "0"
        if protocol.lower() == "rpbob":
            delta = re.findall(r"\d+\.\d+", filename)[0]
            if delta not in PERIODS:
                continue
            if len(PERIODS) > 1:
                protocol_label += rf" Δ = {delta} ms"
        elif protocol.lower() == "beaconing":
            delta = re.search(r"period_(\d+\.\d+)", filename).group(1)
            dprop = re.search(r"dprop_(\d+\.\d+)", filename).group(1)
            if delta not in BCN_PERIODS or dprop not in DPROPS:
                continue
            if len(BCN_PERIODS) > 1 or len(DPROPS) > 1:
                protocol_label += rf" Δ = {delta} ms, dprop = {dprop} ms"

        if csv_path.stat().st_size == 0:
            continue

        # if algebra:
        #     protocol_label += f" {algebra}"
        sort_keys[protocol_label] = (Proto_label.index(base), algebra, float(delta))

        # print(csv_path)

        df_raw = pd.read_csv(csv_path, usecols=lambda c: c in ("failure", "time", "msg"))
        if "failure" in df_raw:
            df_raw["msg"] = df_raw["msg"] / df_raw["failure"].nunique()
        raw_frames[(protocol_label, n)] = df_raw[["time", "msg"]]


max_time = {}
max_t = 0
for (protocol_label, n), df_raw in raw_frames.items():
    if max_t < df_raw["time"].max() + 10:
        max_t = 10 + df_raw["time"].max()
if XMAX is not None:
    max_t = XMAX

for (protocol_label, n), df_raw in raw_frames.items():
    if PAD and df_raw["time"].max() < max_t:
        pad_row = pd.DataFrame({"time": [max_t], "msg": [0]})
        df_raw = pd.concat([df_raw, pad_row], ignore_index=True)

    binned = bin_series(df_raw, bin_freq=f"{BIN}s")
    if EXTRAPOLATE_SYRUP and protocol_label.startswith("SyRuP"):
        binned, ext = extend_periodic(binned, max_t, sort_keys[protocol_label][2])
        if ext is not None:
            binned = pd.concat([binned, ext])

    rows.append(pd.DataFrame({
        "time": binned.index,
        "msg": binned.values,
        "protocol": protocol_label,
        "dataset": n,
    }))

clique_data = pd.concat(rows, ignore_index=True)

for n in NETWORKS:
    df_n = clique_data[clique_data["dataset"] == n].copy()

    if df_n.empty:
        print(f"no data for {n}, skipping")
        continue

    order = sorted(df_n["protocol"].unique(), key=sort_keys.get)
    max_y = df_n["msg"].max() * 10
    colors = dict(zip(order, sns.color_palette("colorblind", n_colors=len(order))))

    fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))
    if SCENARIO == "cutset":
        plt.vlines(x = 100, ymin = -0.1, ymax =max_y, linestyles="--", colors="black")

    sns.lineplot(
        data=df_n,
        x="time",
        y="msg",
        hue="protocol",
        hue_order=order,
        palette=colors,
        style="protocol",
        style_order=order,
        markers=[MARKERS[i % len(MARKERS)] for i in range(len(order))],
        # markers=None,
        alpha=1,
        markersize=8,
        lw=1.5,
    )

    plt.yscale('symlog', linthresh=0.5)
    plt.xlabel('Time [ms]')
    plt.ylabel(r'Rate of messages exchanged')


    plt.ylim([-0.1, df_n["msg"].max() * 10])
    plt.xlim([XMIN, max_t])

    handles, labels = ax.get_legend_handles_labels()
    legend = ax.legend(
        handles, labels,
        loc="upper right",
        fancybox=True, shadow=False
    )
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    plt.grid(alpha=0.3, which='both')
    plt.tight_layout()

    out_path = OUTPUT_DIR / f"msgRate_{DATASET}_{n}_{SCENARIO}_SP.pdf"

    plt.savefig(out_path, format="pdf", dpi=1000)
    print(f"saved {out_path}")

    plt.show()
    plt.close()
