#!/usr/bin/env python3

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
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
    "axes.labelsize": 10,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 9,
})

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "clique"
NETWORKS = ["64"]
SCENARIO = "linkFailure"
Proto_label = ["RRP", "RRP-N", "BGP", "BGP-RCN"]
PERIODS = ["1.0"]
BCN_PERIODS = ["31.0"]  # beaconing: destination announcement period
DPROPS = ["5.0"]  # beaconing: propagation interval
ALGEBRAS = []
BIN = 10
XMIN = 100
PAD = True

# RRP is periodic: once its log stops (the simulation stops issuing routing computations),
# its line is continued at its steady rate, measured on [STEADY_FROM, T_FAIL), and drawn dotted.
T_FAIL = 100  # failure time [ms]
STEADY_FROM = 50  # start of the window where RRP's steady rate is measured [ms]
EXTRAPOLATE_RRP = True

NETWORK_DIR = {
    "clique": "bclique_n{}",
    "triangle": "triangle_n{}",
    "rocketfuel": "{}",
    "scion": "{}",
}

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "rpbob": "RRP",
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
parser.add_argument("--no-pad", dest="pad", action="store_false", default=PAD,
                    help="don't pad lines that end early with zeros")
parser.add_argument("--save", action="store_true", help="also save the plot to data/plots")
args = parser.parse_args()

DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
Proto_label, PERIODS, ALGEBRAS = args.protocols, args.periods, args.algebras
BCN_PERIODS, DPROPS = args.bperiods, args.dprops
BIN, XMIN, PAD = args.bin, args.xmin, args.pad
DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob" and "notif" in filename.lower():
        return "RRP-N"
    return PROTOCOL.get(key, protocol_raw.upper())


def extend_periodic(binned, t_end):
    """RRP: cut the series where it stops issuing computations and continue it at its steady rate.

    Returns (real part, extrapolated part). The stop is the first bin at or after T_FAIL whose rate
    falls below half the steady rate; after it the log only holds messages still in flight.
    """
    steady = binned[(binned.index >= STEADY_FROM) & (binned.index + BIN <= T_FAIL)].mean()
    after = binned[binned.index >= T_FAIL]
    stopped = after[after < 0.5 * steady]
    if stopped.empty or np.isnan(steady):
        return binned, None
    t_stop = stopped.index.min()
    ext = pd.Series(steady, index=np.arange(t_stop, t_end + BIN, BIN))
    return binned[binned.index < t_stop], ext


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
extensions = {}  # (label, n) -> extrapolated RRP series

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

        if csv_path.stat().st_size == 0:  # runs that were interrupted leave empty files behind
            continue

        # if algebra:
        #     protocol_label += f" {algebra}"
        sort_keys[protocol_label] = (Proto_label.index(base), algebra, float(delta))

        # print(csv_path)

        df_raw = pd.read_csv(csv_path, usecols=lambda c: c in ("failure", "time", "msg"))
        # a log can hold several failure runs (e.g. one per cutset): average over them
        if "failure" in df_raw:
            df_raw["msg"] = df_raw["msg"] / df_raw["failure"].nunique()
        raw_frames[(protocol_label, n)] = df_raw[["time", "msg"]]

if not raw_frames:
    raise SystemExit(f"Nothing found in {DATA_DIR} for networks {NETWORKS}, scenario '{SCENARIO}', "
                     f"protocols {Proto_label}, periods {PERIODS}" + (
                         f", algebras {ALGEBRAS}" if ALGEBRAS else "") + ".")

max_time = {}
max_t = 0
for (protocol_label, n), df_raw in raw_frames.items():
    if max_t < df_raw["time"].max() + 10:
        max_t = 10 + df_raw["time"].max()

for (protocol_label, n), df_raw in raw_frames.items():
    if PAD and df_raw["time"].max() < max_t:
        pad_row = pd.DataFrame({"time": [max_t], "msg": [0]})
        df_raw = pd.concat([df_raw, pad_row], ignore_index=True)

    binned = bin_series(df_raw, bin_freq=f"{BIN}s")
    if EXTRAPOLATE_RRP and protocol_label.startswith("RRP"):
        binned, ext = extend_periodic(binned, max_t)
        if ext is not None:
            extensions[(protocol_label, n)] = (binned, ext)

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

    colors = dict(zip(order, sns.color_palette("colorblind", n_colors=len(order))))

    columnwidth = 345.0
    fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.7))

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
        alpha=0.8,
        markersize=8,
        lw=1.5,
    )
    plt.yscale('symlog', linthresh=0.5)
    plt.xlabel('Time [ms]')
    plt.ylabel(r'Rate of messages exchanged')

    top = max([df_n["msg"].max()] + [e.max() for (_, net), (_, e) in extensions.items() if net == n])
    plt.ylim([-0.1, top * 10])  # a decade of headroom on the log axis
    plt.xlim([XMIN, max_t])

    # RRP beyond its log: its steady rate, dotted, joined to its last real point
    for (label, net), (real, ext) in extensions.items():
        if net != n:
            continue
        x = np.r_[real.index[-1], ext.index] if len(real) else ext.index
        y = np.r_[real.values[-1], ext.values] if len(real) else ext.values
        ax.plot(x, y, color=colors[label], ls=":", lw=1.5, alpha=0.8)

    handles, labels = ax.get_legend_handles_labels()
    if any(net == n for (_, net) in extensions):
        handles.append(Line2D([], [], color="0.4", ls=":", lw=1.5))
        labels.append("RRP, periodic (extrap.)")
    legend = ax.legend(
        handles, labels,
        loc="lower left",
        fancybox=True, shadow=False
    )
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    plt.grid(alpha=0.3, which='both')
    plt.tight_layout()

    out_path = OUTPUT_DIR / f"msgRate_{DATASET}_{n}_{SCENARIO}.pdf"
    plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
    print(f"saved {out_path}")

    plt.show()
    plt.close()
