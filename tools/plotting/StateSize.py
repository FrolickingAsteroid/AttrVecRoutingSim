#!/usr/bin/env python3
import matplotlib.pyplot as plt
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import argparse
import re
import os

# sized for LaTeX as in https://scipy-cookbook.readthedocs.io/items/Matplotlib_LaTeX_Examples.html:
# the figure is drawn at the width it takes in the paper and included at that width, e.g.
# \includegraphics[width=0.5\linewidth] side by side (or [width=\linewidth] alone), so it isn't scaled
# and these font sizes are the sizes in the paper: 10 pt like the body text, smaller ticks and legend
LINEWIDTH = 345.0  # pt, from LaTeX: \showthe\linewidth

# side by side, each figure takes half the line. A golden-ratio figure (0.62) is then only ~106 pt high and
# looks flat next to the text, so it is drawn taller. Alone, it spans the line with the golden ratio
SIDE_BY_SIDE = True
WF, HF = (1, 0.85) if SIDE_BY_SIDE else (1.0, 0.5)  # width / LINEWIDTH, height / width

# the same sizes in PointPlt.py, periodPlot.py and StateSize.py: as no figure is scaled, their text
# is then the same size in the paper whatever width each one takes
plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 10,
    "axes.labelsize": 12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 10,
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
Proto_label = ["SyRuP", "SyRuP-LFN", "BGP", "BGP-RCN"]
Algebra_label = ["SP", "All SP", "Product"]
PERIODS = ["1.0"]

# --clique: one group of bars per clique size, one bar per protocol: its mean state per node and destination.
# The bgp / rcn state was only recorded in linkFailure (after the failure), the windows in normal
CLIQUE_SIZES = [4, 8, 16, 32]
CLIQUE_SCENARIO = {
    "window": "normal",
    "bgp": "linkFailure",
    "rcn": "linkFailure",
}

# --focus: the same per focus size, for node u (FOCUS_NODE, as in FocusPlot.py) instead of the mean over nodes,
# with one bar per period of SyRuP: the first plain, the next ones hatched. Same scenarios as the cliques
FOCUS_SIZES = [4, 8, 16, 32]
FOCUS_NODE = 0
FOCUS_PROTOCOLS = ["BGP", "SyRuP"]
FOCUS_PERIODS = ["1.0", "100.0"]

# periods whose window is shown as recorded; every other window is shown one entry smaller
FULL_WINDOW = ["100.0"]

# how a period is named in the legend, when not by its value (e.g. a period that scales with the topology)
PERIOD_NAMES = {"100.0": "2·2ⁿ"}

# bars at every BIN entries (1, 2, 3, ... by default), up to XMAX (default: the largest state)
BIN = 1
XMAX = None

# statistics printed with each plot: share of (destination, node) pairs holding more than THRESHOLD entries
THRESHOLD = 10

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

# state.csv (bgp / rcn) and the window csv (rrp) live in different protocol directories
PROTOCOL = {
    "window": "SyRuP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
}

# same colours as the other scripts: one per protocol family, Δ as the hatch of the bars.
# With several algebras, each protocol and algebra gets its own colour
FAMILIES = ["BGP", "BGP-RCN", "SyRuP", "SyRuP-LFN"]
HATCHES = ['', '////', '....', 'xxxx']

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


def read_state(net_dir, protocol, label, scenario, protocols=None, periods=None):
    """One frame per state file in <net_dir>/<protocol>/<scenario>/ that passes the filters (protocols and periods
    default to Proto_label and PERIODS): one row per (failure,) destination and node, with the number of state
    entries the node keeps for that destination."""
    protocols, periods = protocols or Proto_label, periods or PERIODS
    pattern = f"*_S_{scenario}_*.csv" if protocol == "window" else f"*_S_{scenario}_state.csv"
    for csv_path in (net_dir / protocol / scenario).glob(pattern):
        filename = csv_path.name
        family = "SyRuP-LFN" if protocol == "window" and "notif" in filename else label
        algebra = get_algebra(filename)
        if family not in protocols or algebra not in Algebra_label:
            continue

        period = get_period(filename) if protocol == "window" else ""  # bgp / rcn have none
        if protocol == "window" and period not in periods:
            continue

        df = pd.read_csv(csv_path)
        df = df[df["node"] != df["dest"]]  # the destination keeps no state towards itself

        # runs appended to an existing file (-A) repeat its rows: keep the last run
        key = [c for c in ("failure", "dest", "node") if c in df]
        repeated = df.duplicated(key, keep="last")
        if repeated.any():
            print(f"warning: {csv_path.name}: dropping {repeated.sum()} repeated rows")
            df = df[~repeated]

        if protocol == "window":
            df = df[df["opt_time"].notna()]  # unreachable nodes hold no window
            # the recorded window size counts one entry more than it holds, except for the periods in FULL_WINDOW
            values, seq = df["wind_size"] - (0 if period in FULL_WINDOW else 1), 0
        else:
            seq = df["seq_size"].to_numpy()  # 0 for bgp
            values = df["rib_size"] + df["seq_size"]

        print(csv_path)
        yield pd.DataFrame({"value": values.to_numpy(), "seq": seq, "Family": family, "Period": period,
                            "Algebra": algebra, "node": df["node"].to_numpy()})


def load(network):
    net_dir = ROOT_DIR / 'data' / 'output' / DATASET / network
    rows = [df for protocol, label in PROTOCOL.items() for df in read_state(net_dir, protocol, label, SCENARIO)]
    if not rows:
        raise SystemExit(f"Nothing found for {DATASET}/{network}, scenario '{SCENARIO}', protocols {Proto_label}, "
                         f"algebras {Algebra_label}, periods {PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def load_focus():
    rows = []
    for size in FOCUS_SIZES:
        net_dir = ROOT_DIR / 'data' / 'output' / "focus" / f"focus_n{size}"
        for protocol, label in PROTOCOL.items():
            for df in read_state(net_dir, protocol, label, CLIQUE_SCENARIO[protocol],
                                 protocols=FOCUS_PROTOCOLS, periods=FOCUS_PERIODS):
                rows.append(df[df["node"] == FOCUS_NODE].assign(Size=size))
    if not rows:
        raise SystemExit(f"Nothing found for focus sizes {FOCUS_SIZES}, protocols {FOCUS_PROTOCOLS}, "
                         f"periods {FOCUS_PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def load_clique():
    rows = []
    for size in CLIQUE_SIZES:
        net_dir = ROOT_DIR / 'data' / 'output' / "clique" / f"bclique_n{size}"
        for protocol, label in PROTOCOL.items():
            for df in read_state(net_dir, protocol, label, CLIQUE_SCENARIO[protocol]):
                rows.append(df.assign(Size=size))
    if not rows:
        raise SystemExit(f"Nothing found for cliques {CLIQUE_SIZES}, protocols {Proto_label}, "
                         f"algebras {Algebra_label}, periods {PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def stats(g):
    # one sample per (destination, node) pair; for bgp-rcn the routes and sequence numbers are also given apart
    v = g["value"]
    large = v[v > THRESHOLD]
    return pd.Series({
        "pairs": len(v),
        "entries": v.sum(),
        "mean": v.mean(),
        "routes_mean": (v - g["seq"]).mean() if not g.name[1].startswith("SyRuP") else np.nan,
        "seq_mean": g["seq"].mean() if g.name[1] == "BGP-RCN" else np.nan,
        "median": v.median(),
        "p90": v.quantile(0.9),
        "p99": v.quantile(0.99),
        "min": v.min(),
        "max": v.max(),
        f"above_{THRESHOLD:g}_%": 100 * len(large) / len(v),
        f"above_{THRESHOLD:g}_n": len(large),
        f"above_{THRESHOLD:g}_mean": large.mean(),
    })


def ccdf(data, xs):
    """Share of the samples holding more than x entries, for each x in xs."""
    return (data[:, None] > xs[None, :]).mean(axis=0)


def plot_ccdf(df, network_name):
    family_colors = dict(zip(FAMILIES, sns.color_palette(palette="colorblind", n_colors=len(FAMILIES))))
    algebras = [a for a in Algebra_label if a in set(df["Algebra"])]
    periods = sorted(set(df["Period"]) - {""}, key=float)
    many_algebras, many_periods = len(algebras) > 1, len(periods) > 1

    sorted_groups = sorted(df.groupby(["Family", "Period", "Algebra"]),
                           key=lambda g: (FAMILIES.index(g[0][0]), algebras.index(g[0][2]), float(g[0][1] or 0)))

    # algebras whose state is the same (e.g. BGP keeps one route per neighbour whatever the algebra)
    # share one bar and one legend entry: "BGP SP / Hops"
    groups = []  # ((family, period, [algebras]), group)
    for (family, period, algebra), group in sorted_groups:
        values = np.sort(group["value"].to_numpy())
        for (f, p, algs), g in groups:
            if (f, p) == (family, period) and np.array_equal(np.sort(g["value"].to_numpy()), values):
                algs.append(algebra)
                break
        else:
            groups.append(((family, period, [algebra]), group))

    def name(family, period, algs):
        label = family
        if period and many_periods:
            label = label.ljust(10) + f"Δ = {period} ms"
        if many_algebras:
            label += " " + " / ".join(algs)
        return label

    pad_width = max(len(name(*key)) for key, _ in groups) + 2

    def average(family, group):
        """The mean state, saying what it is made of."""
        if family.startswith("SyRuP"):
            return f"avg  {group['value'].mean():.1}"
        seq = group["seq"].mean()
        routes = group["value"].mean() - seq
        if family == "BGP-RCN":
            return f"avg {routes:.1f} routes + {seq:.1f} seq. numbers"
        return f"avg {routes:.1f}"

    # one group of bars per threshold x, one bar per curve: the share of nodes holding more than x entries
    xmax = XMAX if XMAX is not None else df["value"].max()
    xs = np.arange(BIN, xmax + 1, BIN)
    width = 0.8 * BIN / len(groups)

    fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

    # several algebras: one colour per protocol and algebra (e.g. BGP SP, BGP Hops), Δ stays the hatch
    pairs = list(dict.fromkeys((family, tuple(algs)) for (family, _, algs), _ in groups))
    pair_colors = dict(zip(pairs, sns.color_palette(palette="colorblind", n_colors=len(pairs))))

    for i, ((family, period, algs), group) in enumerate(groups):
        color = pair_colors[family, tuple(algs)] if many_algebras else family_colors[family]
        hatch = HATCHES[periods.index(period) % len(HATCHES)] if period and many_periods else ''

        ax.bar(
            xs + (i - (len(groups) - 1) / 2) * width,
            ccdf(group["value"].to_numpy(), xs),
            width=width,
            color=color,
            hatch=hatch,
            edgecolor="white",  # colour of the hatch lines
            linewidth=0,
            label=name(family, period, algs).ljust(pad_width) + f"({average(family, group)})",
        )

    ax.set_ylim([0, 1.0])
    ax.set_xlim([BIN / 2, xs[-1] + BIN / 2])
    if len(xs) <= 12:
        ax.set_xticks(xs)  # a tick under each group of bars
    else:
        ax.xaxis.get_major_locator().set_params(integer=True)  # entries are counts
    ax.set_xlabel("State size")
    ax.set_ylabel("CCDF")
    ax.set_axisbelow(True)
    ax.grid(alpha=0.3, which="major")

    # 8 pt: at 10 pt the BGP-RCN entry (routes + seq. numbers) is wider than the axes
    legend = ax.legend(loc='upper right', fancybox=True, shadow=False, fontsize=8)
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    plt.tight_layout()  # fits the labels inside the figure, which keeps its size
    out_path = os.path.join(OUTPUT_DIR, f"state_{network_name}_{SCENARIO}.pdf")
    plt.savefig(out_path, format="pdf", dpi=1000)
    plt.show()
    print(f"\tsaved {out_path}\n")


def plot_sizes(df, sizes, xlabel, ylabel, name, with_period):
    """One group of bars per topology size, one bar per protocol (and per period with with_period: the first plain,
    the next ones hatched): the mean of its state over the rows of that size."""
    family_colors = dict(zip(FAMILIES, sns.color_palette(palette="colorblind", n_colors=len(FAMILIES))))
    sizes = [s for s in sizes if s in set(df["Size"])]
    keys = ["Family", "Period"] if with_period else ["Family"]
    means = df.groupby(keys + ["Size"])["value"].mean()
    series = sorted({k if with_period else (k,) for k in means.index.droplevel("Size")},
                    key=lambda k: (FAMILIES.index(k[0]), float(k[1] or 0) if with_period else 0))

    xs = np.arange(len(sizes))
    width = 0.8 / len(series)

    fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

    for i, key in enumerate(series):
        family = key[0]
        period = key[1] if with_period else ""
        periods = sorted({k[1] for k in series if k[0] == family and k[1]}, key=float) if with_period else []
        ax.bar(
            xs + (i - (len(series) - 1) / 2) * width,
            [means.get((*key, s) if with_period else (family, s), np.nan) for s in sizes],
            width=width,
            color=family_colors[family],
            hatch="xxxx" if period and periods.index(period) > 0 else "",
            edgecolor="white",  # colour of the hatch lines
            linewidth=0,
            label=family.ljust(10) + f"Δ = {PERIOD_NAMES.get(period, period)} ms" if period else family,
        )

    ax.set_xticks(xs, [str(s) for s in sizes])
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_axisbelow(True)
    ax.grid(alpha=0.3, which="major", axis="y")

    legend = ax.legend(loc="upper left")
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    plt.tight_layout()  # fits the labels inside the figure, which keeps its size
    out_path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(out_path, format="pdf", dpi=1000)
    plt.show()
    print(f"\tsaved {out_path}\n")


def plot_clique(df):
    """Mean state per node and destination for each clique size. The period isn't named: the state doesn't
    depend on it."""
    plot_sizes(df, CLIQUE_SIZES, "Nodes in Clique Topology", "Avg. State size", "state_clique.pdf", with_period=False)


def plot_focus(df):
    """State of node u for each focus size, one bar per period of SyRuP."""
    xlabel = "Nodes in Focus Topology" if SIDE_BY_SIDE else "Number of nodes in Focus topology"
    plot_sizes(df, FOCUS_SIZES, xlabel, r"State size of node $u$", f"state_focusNode{FOCUS_NODE}.pdf",
               with_period=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot CCDFs of the routing state kept per node and destination.")
    parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
    parser.add_argument("-n", "--networks", nargs="+", default=NETWORKS, help=f"default: {' '.join(NETWORKS)}")
    parser.add_argument("-s", "--scenario", default=SCENARIO, help=f"default: {SCENARIO}")
    parser.add_argument("-p", "--protocols", nargs="+", default=Proto_label, help=f"default: {' '.join(Proto_label)}")
    parser.add_argument("-a", "--algebras", nargs="+", default=Algebra_label,
                        help=f"default: {' '.join(Algebra_label)}")
    parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"RRP periods (default: {' '.join(PERIODS)})")
    parser.add_argument("--bin", type=int, default=BIN, help=f"entries between two groups of bars (default: {BIN})")
    parser.add_argument("--xmax", type=int, default=XMAX, help="last threshold shown (default: the largest state)")
    parser.add_argument("--threshold", type=float, default=THRESHOLD,
                        help=f"statistics: pairs holding more than this many entries (default: {THRESHOLD})")
    parser.add_argument("--clique", action="store_true",
                        help="plot the mean state per clique size instead of the CCDF of a network")
    parser.add_argument("--sizes", nargs="+", type=int, default=CLIQUE_SIZES,
                        help=f"clique sizes (default: {' '.join(map(str, CLIQUE_SIZES))})")
    parser.add_argument("--focus", action="store_true",
                        help="plot the state of node u per focus size instead of the CCDF of a network")
    parser.add_argument("--focus-periods", nargs="+", default=FOCUS_PERIODS,
                        help=f"SyRuP periods for --focus, the first plain, the next hatched "
                             f"(default: {' '.join(FOCUS_PERIODS)})")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    BIN, XMAX, THRESHOLD = args.bin, args.xmax, args.threshold
    CLIQUE_SIZES = args.sizes
    FOCUS_PERIODS = args.focus_periods

    if args.focus:
        df = load_focus()
        print(f"\nfocus, node {FOCUS_NODE}:")
        print(df.pivot_table(index="Size", columns=["Family", "Period"], values="value").to_string())
        plot_focus(df)

    if args.clique:
        df = load_clique()
        table = df.groupby(["Size", "Family", "Period"]).apply(stats, include_groups=False)
        print("\nclique:")
        print(table.round(2).to_string())
        plot_clique(df)

    for network in ([] if args.clique or args.focus else NETWORKS):
        df = load(network)
        table = df.groupby(["Algebra", "Family", "Period"]).apply(stats, include_groups=False)
        print(f"\n{network} {SCENARIO}:")
        print(table.round(2).to_string())
        plot_ccdf(df, network)
