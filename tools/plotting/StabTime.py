#!/usr/bin/env python3
import matplotlib.pyplot as plt
from matplotlib import rc
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.ticker
from matplotlib.lines import Line2D
import itertools
import argparse
import pickle
import glob
import re
import os

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 11,
    "axes.labelsize": 11,
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

# log y axis (x stays linear), labelled 1, 0.1, 0.01, ...
LOGY = False

# log x axis: separates time scales an order of magnitude apart (e.g. RRP with several periods in cutset).
# A time of 0 can't be shown, so a curve starts below 1 when some of its nodes settle at the failure instant
LOGX = False

# one plot per period (with BGP and BGP-RCN in each) instead of every period in one plot
SPLIT_PERIODS = False

# smooth: each curve is evaluated at SMOOTH_POINTS evenly spaced times instead of a step per sample.
# On a log y axis it is also cut once fewer than MIN_LEFT samples remain, so the last few don't drop it by decades
SMOOTH = False
SMOOTH_POINTS = 300
MIN_LEFT = 5

# compact legend: one entry per protocol (colour, with its mean per algebra) and one per algebra (line style)
# instead of one per curve
COMPACT_LEGEND = False

# statistics printed with each plot: share of affected nodes taking longer than THRESHOLD ms
THRESHOLD = 40.0

# keep only the (failure, destination) pairs every loaded protocol was run on (e.g. after an interrupted run)
COMMON = False

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "rpbob": "RRP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
}

# colour order when protocols are coloured by family (several periods of one algebra)
FAMILIES = ["BGP", "BGP-RCN", "RRP", "RRP-N"]
PERIOD_STYLES = ['-', (0, (5, 2)), (0, (1.2, 1.2)), '-.']

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

# protocols whose curves coincide with the plotted ones: named in each legend entry instead of drawn
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
    for csv_path in DATA_DIR.glob(f"{network}/*/{SCENARIO}/*{SCENARIO}*convergence.csv"):
        protocol = csv_path.parent.parent.name
        filename = csv_path.name.lower()

        protocol_label = get_protocol_label(protocol, filename)
        algebra_label = get_algebra(filename)

        # filter by family: under linkFailure the RRP labels carry the period ("RRP   Δ = 1.0 ms")
        if get_family(protocol_label) not in Proto_label:
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

        # an interrupted run leaves a truncated last row behind
        broken = df[["dest", "node", "stab_time", "change_count"]].isna().any(axis=1)
        if broken.any():
            print(f"\tdropping {broken.sum()} incomplete row(s) from {csv_path.name}")
            df = df[~broken]

        # a run appended (-A) to an existing file repeats every (failure, destination, node): keep the first
        keys = [c for c in ("failure", "dest", "node") if c in df]
        repeated = df.duplicated(keys)
        if repeated.any():
            print(f"\tdropping {repeated.sum()} repeated row(s) from {csv_path.name} (appended run?)")
            df = df[~repeated]

        # every node is kept: the unaffected ones (change_count == 0) are needed for the statistics
        df = df[[c for c in ("failure", "dest", "stab_time", "change_count") if c in df]].copy()
        df['Protocol'] = protocol_label
        df['Algebra'] = algebra_label
        rows.append(df)


    df = pd.concat(rows, ignore_index=True)

    if COMMON and "failure" in df:
        # keep only the (failure, destination) pairs every protocol was run on, per algebra
        keep = []
        for algebra, group in df.groupby("Algebra"):
            pairs = [set(zip(g["failure"], g["dest"])) for _, g in group.groupby("Protocol")]
            common = set.intersection(*pairs)
            keep.append(group[[pair in common for pair in zip(group["failure"], group["dest"])]])
            print(f"\t{algebra}: {len(common)} (failure, destination) pairs common to all protocols "
                  f"(of {', '.join(str(len(p)) for p in pairs)})")
        df = pd.concat(keep, ignore_index=True)

    return df


def stats(g):
    # affected: preferred attribute changed. Not stab_time == 0, which also holds for nodes that change
    # exactly at the failure instant. Means, tail share and maximum are over the affected nodes only
    affected = g["change_count"] != 0
    t = g.loc[affected, "stab_time"]
    slow = t[t > THRESHOLD]
    return pd.Series({
        "unaffected_%": 100 * (~affected).mean(),
        "mean_ms": g["stab_time"].mean(),
        "mean_affected_ms": t.mean(),
        "min_ms": t.min(),
        "max_ms": t.max(),
        f"above_{THRESHOLD:g}ms_%": 100 * len(slow) / len(t),
        f"above_{THRESHOLD:g}ms_n": len(slow),
        f"above_{THRESHOLD:g}ms_mean": slow.mean(),
    })


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


# tick label as a module-level function: the figure is pickled, and lambdas can't be
def decimal_label(v, _):
    decimals = max(0, -int(np.floor(np.log10(v))))  # 1, 0.1, 0.01, ... never 1e-05
    return f"{v:.{decimals}f}"


def period_of(label):
    m = re.search(r"Δ = (\d+(?:\.\d+)?)", label)
    return m.group(1) if m else None


def smooth_ccdf(data):
    v = np.sort(data)
    n = len(v)
    x = np.linspace(0.0, v[-1], SMOOTH_POINTS)
    left = n - np.searchsorted(v, x, side="right")  # samples still above x
    keep = left >= MIN_LEFT if LOGY else np.ones_like(left, dtype=bool)
    return x[keep], left[keep] / n


def plot_ccdf(df, network_name, tag=""):
    df = df[df["change_count"] != 0].copy()  # the curves are over the affected nodes
    avg_times = df.groupby(["Protocol", "Algebra"])["stab_time"].mean()

    dfg = (
        df.groupby(["Protocol", "Algebra"])["stab_time"]
        .apply(lambda s: ecdf(s, array=False))
        .reset_index(level=[0, 1])
        .reset_index(drop=True)
    )

    # protocols with coinciding curves share the plotted line and its legend entry
    dfg["name"] = dfg["Protocol"].apply(lambda p: " / ".join([p] + SAME_AS))

    dfg["label_raw"] = dfg.apply(
        lambda row: row["name"].replace(r"\Delta", "Delta").replace("$", "") + " " + row['Algebra'],
        axis=1
    )
    pad_width = dfg["label_raw"].str.len().max() + 2
    dfg["group_label"] = dfg.apply(
        lambda row: (row['name'] + " " + row['Algebra']).ljust(pad_width) +
                    f"({avg_times[(row['Protocol'], row['Algebra'])]:>4.2f} ms)",
        axis=1,
    )

    algebras = sorted(dfg["Algebra"].unique())
    protocols = sorted(dfg["Protocol"].unique())

    colors = dict(zip(protocols, sns.color_palette(palette="colorblind", n_colors=len(protocols))))
    linestyles = dict(zip(algebras, itertools.cycle(['-', 'dotted'])))

    # several periods of a single algebra: colour by protocol family, line style by period
    periods = sorted({period_of(p) for p in protocols} - {None}, key=float)
    by_period = len(algebras) == 1 and len(periods) > 1
    families = [f for f in FAMILIES if any(get_family(p) == f for p in protocols)]
    family_colors = dict(zip(FAMILIES, sns.color_palette(palette="colorblind", n_colors=len(FAMILIES))))
    period_styles = dict(zip(periods, itertools.cycle(PERIOD_STYLES)))
    if by_period:
        colors = {p: family_colors[get_family(p)] for p in protocols}

    columnwidth = 345.0
    fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.75))

    # log y: lowest value a curve can reach is one sample left (MIN_LEFT when smoothed), on the largest curve
    largest_n = df.groupby(["Protocol", "Algebra"]).size().max()
    y_bottom = 10 ** np.floor(np.log10((MIN_LEFT if SMOOTH else 1) / largest_n))

    for algebra in algebras:
        for protocol in protocols:
            group = dfg[(dfg["Algebra"] == algebra) & (dfg["Protocol"] == protocol)]
            if group.empty:
                continue
            if SMOOTH:
                values = df[(df["Algebra"] == algebra) & (df["Protocol"] == protocol)]["stab_time"]
                x, y = smooth_ccdf(values.to_numpy())
                drawstyle = "default"
            else:
                # stable: ecdf() already ordered tied times by decreasing y; an unstable sort shuffles them,
                # and a curve whose largest time repeats can then end on a higher point instead of its last drop
                group = group.sort_values("x", kind="stable")
                x, y = group["x"], group["y"].to_numpy()
                if LOGY:
                    # a log axis can't draw the final 0: end each curve below the axis instead,
                    # so its last drop always runs off the bottom
                    y = np.where(y > 0, y, y_bottom / 10)
                drawstyle = "steps-post"
            ax.plot(
                x, y,
                color=colors[protocol],
                linestyle=period_styles.get(period_of(protocol), '-') if by_period else linestyles[algebra],
                drawstyle=drawstyle,
                alpha=1,
                label=group["group_label"].iloc[0],
                linewidth=2,
            )

    if LOGY:
        ax.set_yscale('log')
        ax.set_ylim([y_bottom, 1.2])
        ax.set_xlim([0.0, df["stab_time"].max() * 1.05])  # the tail is what the log axis is for: show all of it
        ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(decimal_label))
        ax.grid(alpha=0.3, which="major")
    else:
        ax.set_ylim([-1e-2, 1.1])
        ax.set_xlim([0.0, 65])
        ax.grid(alpha=0.3, which="both")
    if LOGX:
        # from the decade holding the shortest 1% of the (non-zero) times to just past the longest
        positive = df.loc[df["stab_time"] > 0, "stab_time"]
        left = np.floor(np.log10(positive.quantile(0.01)))
        if COMPACT_LEGEND and not LOGY:
            left -= 1  # one empty decade on the left, for the means table to sit in
        ax.set_xscale('log')
        ax.set_xlim([10 ** left, df["stab_time"].max() * 1.2])
        ax.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(decimal_label))
        ax.grid(False, which="minor")
        ax.grid(alpha=0.3, which="major")
    ax.set_xlabel("Time [ms]")
    ax.set_ylabel("CCDF")

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
    if COMPACT_LEGEND:
        # two legends: a table of means (one row per protocol, one column per algebra, or per period when
        # several periods of one algebra are plotted), and the line styles side by side in a single row
        header = "mean [ms]"
        if by_period:
            # families without a period (BGP, BGP-RCN) have a single value, under the first column
            columns = [f"Δ={float(d):g}" for d in periods]
            cells = {}
            for p in protocols:
                column = periods.index(period_of(p)) if period_of(p) else 0
                cells.setdefault(get_family(p), {})[column] = f"{avg_times[(p, algebras[0])]:.2f}"
            rows = [(family_colors[f], f, [cells[f].get(c, "-") for c in range(len(periods))]
                     if len(cells[f]) > 1 or 0 not in cells[f] else [cells[f][0]]) for f in families]
            style_handles = [Line2D([], [], color="0.35", linestyle=period_styles[d], linewidth=2) for d in periods]
            style_labels = [f"Δ = {float(d):g} ms" for d in periods]
        else:
            names = dfg.drop_duplicates("Protocol").set_index("Protocol")["name"]
            columns = list(algebras)
            rows = [(colors[p], names[p], [f"{avg_times[(p, a)]:.2f}" if (p, a) in avg_times else "-"
                                           for a in algebras]) for p in protocols]
            style_handles = [Line2D([], [], color="0.35", linestyle=linestyles[a], linewidth=2) for a in algebras]
            style_labels = list(algebras)

        # every cell padded to one width, so the "|" line up in the monospace font
        width = max([len(name) for _, name, _ in rows] + [len(header)]) + 2
        cell = max(len(v) for v in [v for _, _, values in rows for v in values] + columns)

        # header row (no line sample) naming the columns, then one row per protocol
        handles = [Line2D([], [], linestyle="none")]
        labels = [header.ljust(width) + " " + " | ".join(c.center(cell) for c in columns) + " "]
        handles += [Line2D([], [], color=color, linestyle="-", linewidth=2) for color, _, _ in rows]
        labels += [name.ljust(width) + "(" + " | ".join(v.rjust(cell) for v in values) + ")"
                   for _, name, values in rows]
        tight = dict(fancybox=True, shadow=False, handlelength=1.8, handletextpad=0.5,
                     borderpad=0.4, labelspacing=0.3, borderaxespad=0.4)

        # the curves fall from the top left: the upper right stays free on a linear x axis. On a log x axis
        # with a linear y axis the slow curves sit at the top right instead, and the lower left is free
        table_loc, style_loc = ("lower left", "upper right") if LOGX and not LOGY else ("upper right", "lower left")
        if LOGX and not LOGY:
            tight["fontsize"] = 7.5  # the free lower-left corner is small: about 0.1-5 ms, below 0.4
        legend = ax.legend(handles, labels, loc=table_loc, **tight)
        for text in legend.get_texts():
            text.set_fontfamily("monospace")
        ax.add_artist(legend)

        # the line styles side by side, without a box (a single style says nothing)
        if len(style_handles) > 1:
            ax.legend(style_handles, style_labels, loc=style_loc, frameon=False, ncol=len(style_handles))
    else:
        legend = ax.legend(
            handles, labels,
            loc='upper right',
            fancybox=False, shadow=False
        )
        for text in legend.get_texts():
            text.set_fontfamily("monospace")

    suffix = tag + ("_logx" if LOGX else "") + ("_logy" if LOGY else "") + ("_smooth" if SMOOTH else "")
    name = f"ccdf_{network_name}_{SCENARIO}{suffix}.pdf"
    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
    pickle.dump(fig,open(f'raw_plots/ccdf_{network_name}_{SCENARIO}{suffix}','wb'))

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
                        help="protocols whose curves coincide with the plotted ones, named in each legend entry")
    parser.add_argument("--logy", action="store_true", default=LOGY, help="log y axis (1, 0.1, 0.01, ...)")
    parser.add_argument("--split-periods", action="store_true", default=SPLIT_PERIODS,
                        help="one plot per period in --periods, each with BGP and BGP-RCN")
    parser.add_argument("--logx", action="store_true", default=LOGX,
                        help="log x axis, for time scales an order of magnitude apart (e.g. several periods)")
    parser.add_argument("--threshold", type=float, default=THRESHOLD,
                        help=f"statistics: share of affected nodes slower than this [ms] (default: {THRESHOLD:g})")
    parser.add_argument("--common", action="store_true", default=COMMON,
                        help="keep only the (failure, destination) pairs every protocol was run on")
    parser.add_argument("--compact-legend", action="store_true", default=COMPACT_LEGEND,
                        help="one legend entry per protocol (colour) and per algebra (line style)")
    parser.add_argument("--smooth", action="store_true", default=SMOOTH,
                        help=f"draw each curve through {SMOOTH_POINTS} evenly spaced times instead of a step per sample")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    SAME_AS = args.same_as
    LOGY, LOGX, SMOOTH = args.logy, args.logx, args.smooth
    SPLIT_PERIODS = args.split_periods
    COMPACT_LEGEND = args.compact_legend
    THRESHOLD, COMMON = args.threshold, args.common

    print(PERIODS)
    for network in NETWORKS:
        df = load(network)
        table = df.groupby(["Algebra", "Protocol"]).apply(stats, include_groups=False)
        table[f"above_{THRESHOLD:g}ms_n"] = table[f"above_{THRESHOLD:g}ms_n"].astype(int)
        print(f"\n{network} {SCENARIO}:")
        print(table.round(2).to_string())
        if SPLIT_PERIODS:
            # protocols without a period (BGP, BGP-RCN) go into every plot
            for period in PERIODS:
                keep = df["Protocol"].map(lambda p: period_of(p) in (None, period))
                plot_ccdf(df[keep], network, tag=f"_delta{period}")
        else:
            plot_ccdf(df, network)