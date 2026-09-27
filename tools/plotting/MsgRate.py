#!/usr/bin/env python3

import matplotlib.pyplot as plt
from pathlib import Path
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
NETWORKS = ["32"]
SCENARIO = "linkFailure"
Proto_label = ["RRP", "RRP-N", "BGP", "BGP-RCN"]
PERIODS = ["1.0"]
ALGEBRAS = []
BIN = 10
XMIN = 100
PAD = True

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
}

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-explicit": "All SP",
    "lex-hops-explicit": "All Hops",
    "lex-length-hops-suffix": "SP",
    "lex-length-width-suffix": "WSP",
    "prod-width-length-hops-suffix": "Product",
}

MARKERS = ['s', "o", "v", "D", "^", "P", "X", "*"]

parser = argparse.ArgumentParser(description="Plot the rate of messages exchanged over time.")
parser.add_argument("-t", "--topo", default=DATASET, choices=NETWORK_DIR, help=f"default: {DATASET}")
parser.add_argument("-n", "--networks", nargs="+", default=NETWORKS, help=f"default: {' '.join(NETWORKS)}")
parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
parser.add_argument("-p", "--protocols", nargs="+", default=Proto_label, help=f"default: {' '.join(Proto_label)}")
parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"default: {' '.join(PERIODS)}")
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
BIN, XMIN, PAD = args.bin, args.xmin, args.pad
DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob" and "notif" in filename.lower():
        return "RRP-N"
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
    for csv_path in net_dir.glob(f"*/*_S_{SCENARIO}_*msglog.csv"):
        protocol = csv_path.parent.name
        filename = csv_path.name
        protocol_label = get_protocol_label(protocol, csv_path.name)

        if protocol_label not in Proto_label:
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
                protocol_label += rf" $\Delta = {delta}$ ms"

        # if algebra:
        #     protocol_label += f" {algebra}"
        sort_keys[protocol_label] = (Proto_label.index(base), algebra, float(delta))

        print(csv_path)

        df_raw = pd.read_csv(csv_path)[['time', 'msg']]

        max = 0
        conv_path = csv_path.with_name(filename.replace("_msglog.csv", "_convergence.csv"))
        max_time = pd.read_csv(conv_path)["stab_time"].max() if conv_path.exists() else None
        if max_time > max:
            max = max_time

        raw_frames[(protocol_label, n)] = df_raw

if not raw_frames:
    raise SystemExit(f"Nothing found in {DATA_DIR} for networks {NETWORKS}, scenario '{SCENARIO}', "
                     f"protocols {Proto_label}, periods {PERIODS}" + (
                         f", algebras {ALGEBRAS}" if ALGEBRAS else "") + ".")

max_time = {}
max_t = 0
for (protocol_label, n), df_raw in raw_frames.items():
    if max_t < df_raw["time"].max():
        max_t = df_raw["time"].max()

for (protocol_label, n), df_raw in raw_frames.items():
    if PAD and df_raw["time"].max() < max_t:
        pad_row = pd.DataFrame({"time": [max_t], "msg": [0]})
        df_raw = pd.concat([df_raw, pad_row], ignore_index=True)

    binned = bin_series(df_raw, bin_freq=f"{BIN}s")

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

    sns.set_palette("colorblind")

    fig, ax = plt.subplots(figsize=(6, 4))

    sns.lineplot(
        data=df_n,
        x="time",
        y="msg",
        hue="protocol",
        hue_order=order,
        style="protocol",
        style_order=order,
        markers=[MARKERS[i % len(MARKERS)] for i in range(len(order))],
        # markers=None,
        alpha=0.9,
        markersize=8,
        lw=1.5,
    )
    plt.yscale('symlog', linthresh=0.5)
    plt.xlabel('Time [ms]')
    plt.ylabel(r'Rate of messages exchanged')

    plt.ylim([-0.1, df_n["msg"].max() + 10 ** 6])
    plt.xlim([XMIN, max+100])

    handles, labels = ax.get_legend_handles_labels()
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
    plt.savefig(out_path,format="pdf", bbox_inches="tight", dpi=1000)
    print(f"saved {out_path}")

    plt.show()
    plt.close()
