#!/usr/bin/env python3
import matplotlib.pyplot as plt
from matplotlib import rc
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib
import itertools
import argparse
import glob
import re
import os

# plt.rcParams.update({
#     "font.family": "Liberation Sans",
#     "mathtext.fontset": "cm",
#     "font.size": 10,
# })

TEXTWIDTH = 6.30
FIG_W = 10  # 0.49 * TEXTWIDTH
FIG_H = 8  # 0.75 * FIG_W

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

DATASET = "rocketfuel"
NETWORKS = ["1755"]
SCENARIO = "normal"
Proto_label = ["RRP","BGP"]
Algebra_label = ["SP", "Hops"]
PERIODS = ["1.0"]
METRIC = "time"      # time: convergence time (..._convergence.csv) | msgs: messages exchanged per destination (msglog)
LOGX = False         # log time/message axis

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
    "prod-width-length-hops-suffix": "Product ",

}

def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if "notif" in filename:
            if SCENARIO == "linkFailure":
                return rf"RRP-N $\Delta = {delta}$ ms"
            else:
                return rf"RRP-N"
        else:
            if SCENARIO == "linkFailure":
                return rf"RRP $\Delta = {delta}$ ms"
            else:
                return rf"RRP"

    return PROTOCOL.get(key, protocol_raw.upper())


def get_algebra(filename: str) -> str:
    start = 'a_'
    end = '_'

    result = re.search('%s(.*?)%s' % (start, end), filename).group(1)
    return Algebra[result]


def get_family(label: str) -> str:
    if label.startswith("RRP-N"):
        return "RRP-N"
    if label.startswith("RRP"):
        return "RRP"
    return label


def load(network):
    rows = []
    DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET
    kind = "convergence" if METRIC == "time" else "msglog"
    for csv_path in DATA_DIR.glob(f"{network}/*/*_S_{SCENARIO}_*{kind}*.csv"):
        protocol = csv_path.parent.name
        filename = csv_path.name.lower()

        wanted = f"_{kind}_seq_0.csv" if METRIC == "msgs" and protocol.lower() == "rpbob" else f"_{kind}.csv"
        if not filename.endswith(wanted):
            continue

        protocol_label = get_protocol_label(protocol, filename)
        algebra_label = get_algebra(filename)

        if get_family(protocol_label) not in Proto_label:
            continue

        if algebra_label not in Algebra_label:
            continue

        if protocol.lower() == "rpbob" and re.findall(r"\d+\.\d+", filename)[0] not in PERIODS:
            continue

        print(csv_path)

        df = pd.read_csv(csv_path)
        if METRIC == "time":
            df = df[df["change_count"] != 0]
            df = df[df["stab_time"] != 0]
            values = df["stab_time"]
        else:
            values = df.groupby("dest")["msg"].sum()      # messages exchanged per destination
        df = pd.DataFrame({"value": values.to_numpy()})
        df['Protocol'] = protocol_label
        df['Algebra'] = algebra_label
        rows.append(df)

    if not rows:
        raise SystemExit(f"Nothing found for {DATASET}/{network}, scenario '{SCENARIO}', protocols {Proto_label}, "
                         f"algebras {Algebra_label}, periods {PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def ecdf(data, array: bool = True):
    n = len(data)
    x = np.sort(data)
    y = 1 - np.arange(1, n + 1) / n

    x = np.concatenate([[x[0] if LOGX else 0.0], x])     # 0 can't be shown on a log axis
    y = np.concatenate([[1.0], y])
    if not array:
        return pd.DataFrame({'x': x, 'y': y})
    else:
        return x, y


def plot_ccdf(df, network_name):
    df = df.copy()
    unit = "ms" if METRIC == "time" else "msgs"
    avg_times = df.groupby(["Protocol", "Algebra"])["value"].mean()

    dfg = (
        df.groupby(["Protocol", "Algebra"])["value"]
        .apply(lambda s: ecdf(s, array=False))
        .reset_index(level=[0, 1])
        .reset_index(drop=True)
    )

    dfg["label_raw"] = dfg.apply(
        lambda row: row["Protocol"].replace(r"\Delta", "Delta").replace("$", "") + " " + row['Algebra'],
        axis=1
    )
    pad_width = dfg["label_raw"].str.len().max() + 2
    dfg["group_label"] = dfg.apply(
        lambda row: (row['Protocol'] + " " + row['Algebra']).ljust(pad_width) +
                    f"({avg_times[(row['Protocol'], row['Algebra'])]:>8.2f} {unit})",
        axis=1,
    )

    algebras = sorted(dfg["Algebra"].unique())
    protocols = sorted(dfg["Protocol"].unique())

    colors = dict(zip(protocols, sns.color_palette(palette="colorblind", n_colors=len(protocols))))
    linestyles = dict(zip(algebras, itertools.cycle(['-', 'dotted'])))

    fig, ax = plt.subplots(figsize=(FIG_W, FIG_H))

    for algebra in algebras:
        for protocol in protocols:
            group = dfg[(dfg["Algebra"] == algebra) & (dfg["Protocol"] == protocol)]
            if group.empty:
                continue
            group = group.sort_values("x")
            ax.plot(
                group["x"], group["y"],
                color=colors[protocol],
                linestyle=linestyles[algebra],
                drawstyle="steps-post",
                alpha=0.8,
                label=group["group_label"].iloc[0],
                linewidth=2,
            )

    ax.set_ylim([-1e-2, 1.1])
    if LOGX:
        ax.set_xscale("log")
        ax.set_xlim([df["value"].min(), df["value"].max()])
    else:
        ax.set_xlim([0.0, df["value"].max()])
    ax.set_xlabel("Time [ms]" if METRIC == "time" else "Messages exchanged per destination")
    ax.set_ylabel("CCDF")
    ax.grid(alpha=0.3, which="both")

    # shrink plot height to make room for a legend below
    # (https://stackoverflow.com/questions/4700614/how-to-put-the-legend-outside-the-plot)
    # box = ax.get_position()
    # ax.set_position([box.x0, box.y0 + box.height * 0.15,
    #                  box.width, box.height * 0.85])
    #
    # handles, labels = ax.get_legend_handles_labels()
    # legend = ax.legend(
    #     handles, labels,
    #     loc='upper center', bbox_to_anchor=(0.5, -0.15),
    #     fancybox=True, shadow=False, ncol=len(algebras),
    # )

    handles, labels = ax.get_legend_handles_labels()
    legend = ax.legend(
        handles, labels,
        loc='upper right',
        fancybox=True, shadow=False
    )
    # legend.set_title("Protocol")
    # legend.get_title().set_fontweight("bold") if legend.get_title() else None
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    name = f"ccdf_{network_name}_{SCENARIO}.pdf" if METRIC == "time" else f"ccdf_msgs_{network_name}_{SCENARIO}.pdf"
    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
    plt.show()
    print(f"\tsaved {out_path}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot convergence-time CCDFs.")
    parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
    parser.add_argument("-n", "--networks", nargs="+", default=NETWORKS, help=f"default: {' '.join(NETWORKS)}")
    parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
    parser.add_argument("-p", "--protocols", nargs="+", default=Proto_label, help=f"default: {' '.join(Proto_label)}")
    parser.add_argument("-a", "--algebras", nargs="+", default=Algebra_label,
                        help=f"default: {' '.join(Algebra_label)}")
    parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"default: {' '.join(PERIODS)}")
    parser.add_argument("-m", "--metric", choices=["time", "msgs"], default=METRIC,
                        help="time: convergence time; msgs: messages per destination (default: %(default)s)")
    parser.add_argument("--logx", action="store_true", default=LOGX, help="log x axis")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    METRIC, LOGX = args.metric, args.logx

    for network in NETWORKS:
        df = load(network)
        plot_ccdf(df, network)