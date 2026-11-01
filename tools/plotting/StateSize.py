#!/usr/bin/env python3
import matplotlib.pyplot as plt
import matplotlib.ticker
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

# SyRuP periods whose window on focus_n<n> is computed instead of read: 2^n for Δ = 1
FOCUS_COMPUTED = {"1.0": lambda n: 2 ** n}

# log y axis on the focus plot: the computed windows reach ~10^10, next to BGP's n and the 2 of long periods
FOCUS_LOGY = True

# clique / focus: periods whose window is shown as recorded; every other window is shown one entry smaller.
# The network CCDF always shows the windows as recorded
FULL_WINDOW = ["100.0"]

# how a period is named in the legend, when not by its value (e.g. a period that scales with the topology)
PERIOD_NAMES = {"100.0": "2^n"}

# bars at every BIN entries (1, 2, 3, ... by default), up to XMAX (default: the largest state)
BIN = 1
XMAX = None
LOG_BINS = False  # bars at 1, 2, 4, 8, ... instead: for states of very different sizes (e.g. SyRuP and Beaconing)
AT_LEAST = False  # bars: share of nodes holding at least x entries instead of more than x (a state of 1 shows at 1)
LOGY = False  # log y axis, labelled 1, 0.1, 0.01, ...: small shares (a short tail) stay visible

# statistics printed with each plot: share of (destination, node) pairs holding more than THRESHOLD entries
THRESHOLD = 10

OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

# state.csv (bgp / rcn), the window csv (rrp) and the progress log (beaconing) live in different protocol directories
PROTOCOL = {
    "window": "SyRuP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
    "beaconing": "Beaconing",
}

# Beaconing (-p Beaconing): its state is the beacon store at the end of the run, counted in the progress log
# (too large to load: streamed once and cached next to it in *_beaconstate.csv). The store only grows during a run,
# so its last size is also its largest. candidates: every beacon stored | best: the best set it propagates (<= 20)
BCN_PERIODS = ["31.0"]  # destination announcement period
DPROPS = ["5.0"]  # propagation interval
BEACON_STATE = "candidates"

# same colours as the other scripts: one per protocol family, Δ as the hatch of the bars.
# With several algebras, each protocol and algebra gets its own colour
FAMILIES = ["BGP", "BGP-RCN", "SyRuP", "SyRuP-LFN", "Beaconing"]
HATCHES = ['', '////', '....', 'xxxx']

Algebra = {
    "lex-hops-suffix": "Hops",
    "lex-length-explicit": "All SP",
    "lex-hops-explicit": "All Hops",
    "lex-length-hops-suffix": "SP",
    "lex-length-width-suffix": "WSP",
    "prod-width-length-hops-suffix": "Product",
}

# networks (rocketfuel, ...): an algebra's runs on every delay graph are counted together (e.g. SP on DelayLength and
# on DelayHops: 1755 SyRuP SP averages 3.8). MATCH_DELAY keeps only the runs on the algebra's own delay graph,
# the prefix of its files in DELAY_OF (SP on DelayLength alone: 4.4)
MATCH_DELAY = False
DELAY_OF = {"Hops": "DelayHops", "All Hops": "DelayHops"}  # every other algebra: DelayLength


def get_algebra(filename: str) -> str:
    m = re.search(r"_A_(.+?)_S_", filename)
    return Algebra.get(m.group(1), m.group(1)) if m else ""


def get_period(filename: str) -> str:
    m = re.search(r"period_(\d+\.\d+)", filename)
    return m.group(1) if m else ""


def beacon_state(progress):
    """Beacon store of every (destination, node) at the end of the run: the sizes of its best set and candidates."""
    cache = progress.with_name(progress.name.replace("_progress.csv", "_beaconstate.csv"))
    if not cache.exists() or cache.stat().st_mtime < progress.stat().st_mtime:
        print(f"streaming {progress.name} (once, cached in {cache.name})")
        last = {}
        with open(progress, "rb") as f:
            next(f)  # header: dest,node,time,opt,wind,tag (opt: best set, wind: candidates, "; " between beacons)
            for line in f:
                dest, node, _, best, candidates, _ = line.split(b",")
                last[dest, node] = (best.count(b";") + 1 if best else 0,
                                    candidates.count(b";") + 1 if candidates else 0)
        pd.DataFrame([(int(d), int(n), b, c) for (d, n), (b, c) in last.items()],
                     columns=["dest", "node", "best", "candidates"]).to_csv(cache, index=False)
    return pd.read_csv(cache)


def read_state(net_dir, protocol, label, scenario, protocols=None, periods=None, match_delay=False,
               full_window=False):
    """One frame per state file in <net_dir>/<protocol>/<scenario>/ that passes the filters (protocols and periods
    default to Proto_label and PERIODS; match_delay: only the files of each algebra's delay graph, DELAY_OF;
    full_window: every window as recorded): one row per (failure,) destination and node, with the number of state
    entries the node keeps for that destination."""
    protocols, periods = protocols or Proto_label, periods or PERIODS
    pattern = {"window": f"*_S_{scenario}_*.csv", "beaconing": f"*_S_{scenario}_*progress.csv"}.get(
        protocol, f"*_S_{scenario}_state.csv")
    for csv_path in (net_dir / protocol / scenario).glob(pattern):
        filename = csv_path.name
        family = "SyRuP-LFN" if protocol == "window" and "notif" in filename else label
        algebra = get_algebra(filename)
        if family not in protocols or algebra not in Algebra_label:
            continue
        if match_delay and not filename.startswith(DELAY_OF.get(algebra, "DelayLength") + "_"):
            continue

        period = get_period(filename) if protocol == "window" else ""  # the others aren't told apart by period
        if protocol == "window" and period not in periods:
            continue

        if protocol == "beaconing":
            dprop = re.search(r"dprop_(\d+\.\d+)", filename).group(1)
            if get_period(filename) not in BCN_PERIODS or dprop not in DPROPS or csv_path.stat().st_size == 0:
                continue  # interrupted runs leave empty files behind
            df = beacon_state(csv_path)
        else:
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
            # clique / focus: the recorded window size counts one entry more than it holds, except for FULL_WINDOW
            values, seq = df["wind_size"] - (0 if full_window or period in FULL_WINDOW else 1), 0
        elif protocol == "beaconing":
            values, seq = df[BEACON_STATE], 0
        else:
            seq = df["seq_size"].to_numpy()  # 0 for bgp
            values = df["rib_size"] + df["seq_size"]

        print(csv_path)
        yield pd.DataFrame({"value": values.to_numpy(), "seq": seq, "Family": family, "Period": period,
                            "Algebra": algebra, "node": df["node"].to_numpy()})


def load(network):
    net_dir = ROOT_DIR / 'data' / 'output' / DATASET / network
    rows = [df for protocol, label in PROTOCOL.items()
            for df in read_state(net_dir, protocol, label, SCENARIO, match_delay=MATCH_DELAY, full_window=True)]
    if not rows:
        raise SystemExit(f"Nothing found for {DATASET}/{network}, scenario '{SCENARIO}', protocols {Proto_label}, "
                         f"algebras {Algebra_label}, periods {PERIODS}.")
    return pd.concat(rows, ignore_index=True)


def load_focus():
    rows = []
    for size in FOCUS_SIZES:
        net_dir = ROOT_DIR / 'data' / 'output' / "focus" / f"focus_n{size}"
        for protocol, label in PROTOCOL.items():
            if protocol not in CLIQUE_SCENARIO:  # e.g. beaconing: not run on these topologies
                continue
            for df in read_state(net_dir, protocol, label, CLIQUE_SCENARIO[protocol],
                                 protocols=FOCUS_PROTOCOLS, periods=FOCUS_PERIODS):
                rows.append(df[df["node"] == FOCUS_NODE].assign(Size=size))
    if not rows:
        raise SystemExit(f"Nothing found for focus sizes {FOCUS_SIZES}, protocols {FOCUS_PROTOCOLS}, "
                         f"periods {FOCUS_PERIODS}.")
    df = pd.concat(rows, ignore_index=True)

    for period, window in FOCUS_COMPUTED.items():
        computed = (df["Family"] == "SyRuP") & (df["Period"] == period)
        df.loc[computed, "value"] = df.loc[computed, "Size"].map(window)
    return df


def load_clique():
    rows = []
    for size in CLIQUE_SIZES:
        net_dir = ROOT_DIR / 'data' / 'output' / "clique" / f"bclique_n{size}"
        for protocol, label in PROTOCOL.items():
            if protocol not in CLIQUE_SCENARIO:  # e.g. beaconing: not run on these topologies
                continue
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
    """Share of the samples holding more than x entries (at least x with AT_LEAST), for each x in xs."""
    if AT_LEAST:
        return (data[:, None] >= xs[None, :]).mean(axis=0)
    return (data[:, None] > xs[None, :]).mean(axis=0)


# tick label 1, 0.1, 0.01, ... never 1e-05
def decimal_label(v, _):
    return f"{v:.{max(0, -int(np.floor(np.log10(v))))}f}"


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
            return f"avg  {group['value'].mean():.1f}"
        seq = group["seq"].mean()
        routes = group["value"].mean() - seq
        if family == "BGP-RCN":
            return f"avg {routes:.1f} routes + {seq:.1f} seq. numbers"
        return f"avg {routes:.1f}"

    # one group of bars per threshold x, one bar per curve: the share of nodes holding more than x entries.
    # Thresholds every BIN entries, or at 1, 2, 4, 8, ... with LOG_BINS (evenly spaced on the axis)
    xmax = XMAX if XMAX is not None else df["value"].max()
    if LOG_BINS:
        xs = 2 ** np.arange(0, int(np.ceil(np.log2(xmax))) + 1)
        pos, step = np.arange(len(xs)), 1
    else:
        xs = np.arange(BIN, xmax + 1, BIN)
        pos, step = xs, BIN
    width = 0.8 * step / len(groups)

    fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

    # several algebras: one colour per protocol and algebra (e.g. BGP SP, BGP Hops), Δ stays the hatch
    pairs = list(dict.fromkeys((family, tuple(algs)) for (family, _, algs), _ in groups))
    pair_colors = dict(zip(pairs, sns.color_palette(palette="colorblind", n_colors=len(pairs))))

    for i, ((family, period, algs), group) in enumerate(groups):
        color = pair_colors[family, tuple(algs)] if many_algebras else family_colors[family]
        hatch = HATCHES[periods.index(period) % len(HATCHES)] if period and many_periods else ''

        ax.bar(
            pos + (i - (len(groups) - 1) / 2) * width,
            ccdf(group["value"].to_numpy(), xs),
            width=width,
            color=color,
            hatch=hatch,
            edgecolor="white",  # colour of the hatch lines
            linewidth=0,
            label=name(family, period, algs).ljust(pad_width) + f"({average(family, group)})",
        )

    if LOGY:
        # down to the decade of a single sample of the largest curve, so every non-empty bar shows
        largest_n = max(len(group) for _, group in groups)
        ax.set_yscale("log")
        ax.set_ylim([10 ** np.floor(np.log10(1 / largest_n)), 1.0])
        ax.yaxis.set_major_formatter(plt.FuncFormatter(decimal_label))
    else:
        ax.set_ylim([0, 1.0])
    ax.set_xlim([pos[0] - step / 2, pos[-1] + step / 2])
    if LOG_BINS or len(xs) <= 12:
        ax.set_xticks(pos, [str(x) for x in xs])  # a tick under each group of bars
    else:
        ax.xaxis.get_major_locator().set_params(integer=True)  # entries are counts
    ax.set_xlabel("State size")
    ax.set_ylabel("CCDF")
    ax.set_axisbelow(True)
    ax.grid(alpha=0.3, which="major")

    # 8 pt: at 10 pt the BGP-RCN entry (routes + seq. numbers) is wider than the axes
    legend = ax.legend(loc='upper right', fancybox=True, shadow=False)
    for text in legend.get_texts():
        text.set_fontfamily("monospace")

    plt.tight_layout()  # fits the labels inside the figure, which keeps its size
    out_path = os.path.join(OUTPUT_DIR, f"state_{network_name}_{SCENARIO}.pdf")
    plt.savefig(out_path, format="pdf", dpi=1000)
    plt.show()
    print(f"\tsaved {out_path}\n")


def plot_sizes(df, sizes, xlabel, ylabel, name, with_period, logy=False):
    """One group of bars per topology size, one bar per protocol (and per period with with_period: the first plain,
    the next ones hatched): the mean of its state over the rows of that size. logy: log y axis from 1, labelled
    10^0, 10^2, ..."""
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

    if logy:
        top = means.max() * 1e3  # headroom for the legend above the bars
        ax.set_yscale("log")
        ax.set_ylim(1, top)
        ax.yaxis.set_major_locator(matplotlib.ticker.LogLocator(base=10, numticks=8))
        ax.yaxis.set_major_formatter(matplotlib.ticker.LogFormatterMathtext())
        ax.yaxis.set_minor_locator(matplotlib.ticker.NullLocator())

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
               with_period=True, logy=FOCUS_LOGY)


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
    parser.add_argument("--log-bins", action="store_true", default=LOG_BINS,
                        help="bars at 1, 2, 4, 8, ... entries instead of every --bin entries")
    parser.add_argument("--at-least", action="store_true", default=AT_LEAST,
                        help="bars: share of nodes holding at least x entries instead of more than x")
    parser.add_argument("--logy", action="store_true", default=LOGY, help="log y axis (1, 0.1, 0.01, ...)")
    parser.add_argument("--match-delay", action="store_true", default=MATCH_DELAY,
                        help="only each algebra's runs on its own delay graph (SP: DelayLength, Hops: DelayHops)")
    parser.add_argument("--threshold", type=float, default=THRESHOLD,
                        help=f"statistics: pairs holding more than this many entries (default: {THRESHOLD})")
    parser.add_argument("--clique", action="store_true",
                        help="plot the mean state per clique size instead of the CCDF of a network")
    parser.add_argument("--sizes", nargs="+", type=int, default=CLIQUE_SIZES,
                        help=f"clique sizes (default: {' '.join(map(str, CLIQUE_SIZES))})")
    parser.add_argument("--focus", action="store_true",
                        help="plot the state of node u per focus size instead of the CCDF of a network")
    parser.add_argument("--beacon-state", choices=["candidates", "best"], default=BEACON_STATE,
                        help=f"Beaconing: count every stored beacon or only the best set (default: {BEACON_STATE})")
    parser.add_argument("--bperiods", nargs="+", default=BCN_PERIODS,
                        help=f"Beaconing announcement periods (default: {' '.join(BCN_PERIODS)})")
    parser.add_argument("--dprops", nargs="+", default=DPROPS,
                        help=f"Beaconing propagation intervals (default: {' '.join(DPROPS)})")
    parser.add_argument("--focus-periods", nargs="+", default=FOCUS_PERIODS,
                        help=f"SyRuP periods for --focus, the first plain, the next hatched "
                             f"(default: {' '.join(FOCUS_PERIODS)})")
    args = parser.parse_args()

    DATASET, NETWORKS, SCENARIO = args.topo, args.networks, args.scenario
    Proto_label, Algebra_label, PERIODS = args.protocols, args.algebras, args.periods
    BIN, XMAX, THRESHOLD, LOG_BINS = args.bin, args.xmax, args.threshold, args.log_bins
    AT_LEAST, LOGY, MATCH_DELAY = args.at_least, args.logy, args.match_delay
    CLIQUE_SIZES = args.sizes
    FOCUS_PERIODS = args.focus_periods
    BEACON_STATE, BCN_PERIODS, DPROPS = args.beacon_state, args.bperiods, args.dprops

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
