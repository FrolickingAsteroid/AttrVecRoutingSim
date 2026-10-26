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
import pickle
import glob
import re
import os

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
Proto_label = ["RRP"]
Algebra_label = ["SP", "All SP", "Hops", "All Hops"]
PERIODS = ["1.0"]

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

Algebra_label = ["SP", "All SP", "Hops", "All Hops"]
Proto_label = ["RRP"]

# protocols whose curves coincide with the plotted ones: named in the legend title instead of drawn
SAME_AS = []


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


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if "notif" in filename:
            if SCENARIO == "linkFailure":
                return rf"RRP-N Δ = {delta} ms"
            else:
                return rf"RRP-N"
        else:
            if SCENARIO == "linkFailure":
                return rf"RRP".ljust(6) + f"Δ = {delta} ms"
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
    for csv_path in DATA_DIR.glob(f"{network}/*/*{SCENARIO}*convergence.csv"):
        protocol = csv_path.parent.name
        filename = csv_path.name.lower()

        protocol_label = get_protocol_label(protocol, filename)
        algebra_label = get_algebra(filename)

        if protocol_label not in Proto_label:
            continue

        if algebra_label not in Algebra_label:
            continue

        if protocol.lower() == "rpbob":
            delta = re.findall(r"\d+\.\d+", filename)[0]
            if delta not in PERIODS:
                continue
            if len(PERIODS) > 1:
                protocol_label += rf" Δ = {delta} ms"

        print(csv_path)

        df = pd.read_csv(csv_path)
        df = df[df["change_count"] != 0]
        df = df[df["stab_time"] != 0]
        df = df[['stab_time']].copy()
        df['Protocol'] = protocol_label
        df['Algebra'] = algebra_label
        rows.append(df)

    return pd.concat(rows, ignore_index=True)


def ecdf(data, array: bool = True):
    n = len(data)
    x = np.sort(data)
    y = 1 - np.arange(1, n + 1) / n

    x = np.concatenate([[0.0], x])
    y = np.concatenate([[1.0], y])
    if not array:
        return pd.DataFrame({'x': x, 'y': y})
    else:
        return x, y


def plot_ccdf(df, network_name):
    df = df.copy()
    avg_times = df.groupby(["Protocol", "Algebra"])["stab_time"].mean()

    dfg = (
        df.groupby(["Protocol", "Algebra"])["stab_time"]
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
                    f"({avg_times[(row['Protocol'], row['Algebra'])]:>4.2f} ms)",
        axis=1,
    )

    algebras = sorted(dfg["Algebra"].unique())
    protocols = sorted(dfg["Protocol"].unique())

    colors = dict(zip(protocols, sns.color_palette(palette="colorblind", n_colors=len(protocols))))
    linestyles = dict(zip(algebras, itertools.cycle(['-', 'dotted'])))

    columnwidth = 345.0
    fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.7))

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
                alpha=1,
                label=group["group_label"].iloc[0],
                linewidth=2,
            )

    ax.set_ylim([-1e-2, 1.1])
    #ax.set_xscale('log')
    ax.set_xlim([0.0, 500])
    ax.set_xlabel("Time [ms]")
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
        fancybox=True, shadow=False,
        title=("Same for " + (", ".join(SAME_AS[:-1]) + " and " if len(SAME_AS) > 1 else "") + SAME_AS[-1])
        if SAME_AS else None,
    )
    # legend.set_title("Protocol")
    # legend.get_title().set_fontweight("bold") if legend.get_title() else None
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    name = f"ccdf_{network_name}_{SCENARIO}.pdf"
    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
    pickle.dump(fig,open(f'raw_plots/ccdf_{network_name}_{SCENARIO}','wb'))

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
    parser.add_argument("--same-as", nargs="+", default=SAME_AS,
                        help="protocols whose curves coincide with the plotted ones, named in the legend title")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    SAME_AS = args.same_as

    print(PERIODS)
    for network in NETWORKS:
        df = load(network)
        plot_ccdf(df, network)
