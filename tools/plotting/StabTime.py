#!/usr/bin/env python3
import matplotlib.pyplot as plt
from matplotlib import rc
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib
import itertools
import glob
import re
import os

plt.rcParams.update({
    "font.family": "Liberation Sans",
    "mathtext.fontset": "cm",
    "font.size": 10,
})

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "rocketfuel"
SCENARIO = "linkFailure"
DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "rpbob": "RRP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
}

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-hops-suffix": "SP",
    "lex-length-width-suffix": "WSP",
    "prod-width-length-hops-suffix": "Product ",

}

Algebra_label = ["SP"]
Proto_label = ["BGP","RRP"]


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        delta = re.findall(r"\d+\.\d+", filename)[0]
        if "notif" in filename:
            if SCENARIO == "linkFailure":
                return rf"RRP-N"  # $\Delta = {delta}$ ms
            else:
                return rf"RRP-N"
        else:
            if SCENARIO == "linkFailure":
                return rf"RRP"  # $\Delta = {delta}$ ms
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
    for csv_path in DATA_DIR.glob(f"{network}/*/*{SCENARIO}*convergence.csv"):
        protocol = csv_path.parent.name
        filename = csv_path.name.lower()

        protocol_label = get_protocol_label(protocol, filename)
        algebra_label = get_algebra(filename)

        if protocol.lower() == "rpbob" and re.findall(r"\d+\.\d+", filename)[0] != "1.0":
            continue

        if algebra_label not in Algebra_label or protocol_label not in Proto_label:
            continue

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
                    f"({avg_times[(row['Protocol'], row['Algebra'])]:>6.2f} ms)",
        axis=1,
    )

    algebras = sorted(dfg["Algebra"].unique())
    protocols = sorted(dfg["Protocol"].unique())

    colors = dict(zip(protocols, sns.color_palette("colorblind",n_colors=len(protocols))))
    linestyles = dict(zip(algebras, itertools.cycle(['-', '--'])))

    fig, ax = plt.subplots(figsize=(6, 4))

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
                linewidth=1.8,
            )

    # ax.set_yscale("log")
    ax.set_ylim([-1e-2, 1.1])
    ax.set_xlim([0.0, df["stab_time"].max()])
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
        fancybox=True, shadow=False
    )
    #legend.set_title("Protocol")
    legend.get_title().set_fontweight("bold") if legend.get_title() else None
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    name = f"ccdf_{network_name}_{SCENARIO}.png"
    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, dpi=300, bbox_inches="tight")
    plt.close()
    print(f"\tsaved {out_path}\n")


if __name__ == "__main__":
    for network in ["1755"]:
        df = load(network)
        plot_ccdf(df, network)
