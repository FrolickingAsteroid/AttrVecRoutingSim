#!/usr/bin/env python3
"""Rate of messages of SyRuP against beaconing (normal scenario).

Beaconing nodes all propagate at multiples of dprop, so its messages are sent in bursts: each burst is
binned in BIN ms like SyRuP: with BIN <= dprop a burst falls in a single bin, so its spikes show.
Drawn like MsgRate.py: same fonts, size, colours, markers and axes.
"""

import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np
import seaborn as sns
import pandas as pd
import argparse
import re

# the same sizes as MsgRate.py (and the other plotting scripts)
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

DATASET = "rocketfuel"
NETWORK = "1755"
SCENARIO = "normal"
DELAY = "DelayLength"
ALGEBRA = "lex-length-hops-suffix"
PERIODS = ["10.0", "70.0"]  # SyRuP
BCN_PERIOD = "31.0"         # beaconing: destination announcement period
DPROP = "5.0"               # beaconing: propagation interval
BIN = 1  # [ms]
XMAX = 100  # right edge of the x axis

# figure size, as in MsgRate.py: LINEWIDTH is the width the figure takes in the paper, in pt (\showthe\linewidth)
LINEWIDTH = 345.0

SIDE_BY_SIDE = True
WF, HF = (1, 0.85) if SIDE_BY_SIDE else (1.0, (5. ** 0.5 - 1.0) / 2.0)  # width / LINEWIDTH, height / width

DATA_DIR = ROOT_DIR / 'data' / 'output'
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

MARKERS = ['s', "o", "v", "D", "^", "P", "X", "*"]


# Source - https://stackoverflow.com/a/31527287
# Posted by rth, modified by community. See post 'Timeline' for change history
# Retrieved 2026-09-27, License - CC BY-SA 4.0

def get_figsize(columnwidth, wf=0.5, hf=(5. ** 0.5 - 1.0) / 2.0, ):
    fig_width_pt = columnwidth * wf
    inches_per_pt = 1.0 / 72.27  # Convert pt to inch
    fig_width = fig_width_pt * inches_per_pt  # width in inches
    fig_height = fig_width * hf  # height in inches
    return [fig_width, fig_height]


parser = argparse.ArgumentParser(description="Plot the rate of messages of SyRuP against the bursts of beaconing.")
parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
parser.add_argument("-n", "--network", default=NETWORK, help=f"default: {NETWORK}")
parser.add_argument("-d", "--delay", default=DELAY, help=f"default: {DELAY}")
parser.add_argument("-a", "--algebra", default=ALGEBRA, help=f"default: {ALGEBRA}")
parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"SyRuP periods (default: {' '.join(PERIODS)})")
parser.add_argument("--bperiod", default=BCN_PERIOD, help=f"beaconing announcement period (default: {BCN_PERIOD})")
parser.add_argument("--dprop", default=DPROP, help=f"beaconing propagation interval (default: {DPROP})")
parser.add_argument("--bin", type=float, default=BIN, help=f"bin width [ms] (default: {BIN})")
parser.add_argument("--xmax", type=float, default=XMAX, help="right edge of the x axis")
parser.add_argument("--no-repeat-first", dest="repeat_first", action="store_false",
                    help="don't continue a SyRuP log that ends within its first period (e.g. Δ = 70 ms)")
args = parser.parse_args()

net_dir = DATA_DIR / args.topo / args.network
prefix = f"{args.delay}_A_{args.algebra}_"


def read_log(path):
    if not path.exists() or path.stat().st_size == 0:  # interrupted runs leave empty files behind
        raise SystemExit(f"no msglog at {path}")
    return pd.read_csv(path, usecols=["time", "msg"])


bcn_path = net_dir / "beaconing" / SCENARIO / f"{prefix}S_{SCENARIO}_dprop_{args.dprop}_period_{args.bperiod}_msglog.csv"
bursts = read_log(bcn_path).groupby("time")["msg"].sum()
xmax = args.xmax if args.xmax is not None else bursts.index.max() + 10
edges = np.arange(0, xmax + args.bin, args.bin)


def bin_series(series):
    return series.groupby(series.index // args.bin * args.bin).sum().reindex(edges, fill_value=0)


# SyRuP is periodic: once its log stops, its line is continued with its periodic pattern, as in MsgRate.py
T_FAIL = 100  # [ms]
STEADY_FROM = 50  # start of the window where SyRuP's steady rate is measured [ms]
EXTRAPOLATE_SYRUP = True

# a log that ends within the first period (e.g. Δ = 70 ms, run stopped at convergence ~62 ms) has no complete period
# to repeat. The run stops once the announcement's wave has died out, so the rest of that period is silent: it is
# taken as complete (silent after the log) and repeated from Δ, 2Δ, ...
REPEAT_FIRST_PERIOD = args.repeat_first  # default True, --no-repeat-first to turn it off


def extend_periodic(binned, t_end, period):
    """SyRuP: cut the series where it stops issuing computations and continue its pattern (copied from MsgRate.py,
    so both draw the same line).

    Returns (real part, extrapolated part). Periods up to one bin give a steady rate, measured on
    [STEADY_FROM, T_FAIL): the stop is the first bin at or after T_FAIL whose rate falls below half of it,
    after which the log only holds messages still in flight. Longer periods send one burst per
    announcement (at multiples of the period): the log is kept to its end, and its last complete period
    is repeated from there, aligned to the announcements.
    """
    BIN = args.bin
    if period > BIN:
        t_stop = binned[binned > 0].index.max() + BIN  # first bin past the end of the log
        last = int(t_stop // period) - 1  # last period that ends before the log does
        if last < 0:
            if not REPEAT_FIRST_PERIOD:
                return binned, None
            last = 0  # the first period, silent after the end of the log (binned holds zeros there)
        start = last * period
        template = binned[(binned.index >= start) & (binned.index < start + period)]
        times = np.arange(t_stop, t_end + BIN, BIN)
        phase = start + ((times - start) % period) // BIN * BIN
        ext = pd.Series(template.reindex(phase, fill_value=0).to_numpy(), index=times)
        return binned[binned.index < t_stop], ext

    steady = binned[(binned.index >= STEADY_FROM) & (binned.index + BIN <= T_FAIL)].mean()
    after = binned[binned.index >= T_FAIL]
    stopped = after[after < 0.5 * steady]
    if stopped.empty or np.isnan(steady):
        return binned, None
    t_stop = stopped.index.min()
    ext = pd.Series(steady, index=np.arange(t_stop, t_end + BIN, BIN))
    return binned[binned.index < t_stop], ext


syrup = {}
for period in args.periods:
    df = read_log(net_dir / "rpbob" / SCENARIO / f"{prefix}N_adaptative_S_{SCENARIO}_period_{period}_msglog.csv")
    syrup[period] = bin_series(df.groupby("time")["msg"].sum())

# summed over all destinations, over the time beaconing runs
t_end = bursts.index.max()
print(f"messages in [0, {t_end:g}] ms, summed over destinations")
print(f"  {'Beaconing':<16} total {bursts.sum():>12,.0f}   {bursts.sum() / t_end:>10,.0f} /ms   "
      f"largest burst {bursts.max():>10,.0f} at {bursts.idxmax():g} ms")
for period, binned in syrup.items():
    s = binned[binned.index <= t_end]
    print(f"  {f'SyRuP Δ = {period}':<16} total {s.sum():>12,.0f}   {s.sum() / t_end:>10,.0f} /ms")

if EXTRAPOLATE_SYRUP:
    for period, binned in syrup.items():
        # the continuation is part of the series: drawn exactly like the logged part
        real, ext = extend_periodic(binned, xmax, float(period))
        if ext is not None:
            syrup[period] = pd.concat([real, ext]).reindex(edges, fill_value=0)

# drawn like MsgRate.py: one line per protocol, binned. Beaconing first: first in the legend, and drawn first,
# under the SyRuP lines
rows = [pd.DataFrame({"time": edges, "msg": bin_series(bursts).values, "protocol": "Beaconing"})]
rows += [pd.DataFrame({"time": b.index, "msg": b.values,
                       "protocol": f"SyRuP Δ = {p} ms" if len(syrup) > 1 else "SyRuP"}) for p, b in syrup.items()]
df_n = pd.concat(rows, ignore_index=True)

order = list(df_n["protocol"].unique())

# the protocols' colours in StateSize.py (colorblind palette: BGP, BGP-RCN, SyRuP, SyRuP-LFN, Beaconing):
# Beaconing pink, SyRuP green with the triangle of the point plots; periods in PERIOD_STYLE get their own
palette = sns.color_palette("colorblind", n_colors=5)
BEACONING_STYLE = (palette[0], "s")
SYRUP_STYLE = (palette[2], "v")
PERIOD_STYLE = {"70.0": (palette[3], "D")}  # SyRuP Δ = 70 ms: red diamond


def style_of(label):
    if label == "Beaconing":
        return BEACONING_STYLE
    period = re.search(r"Δ = (\d+\.\d+)", label)
    return PERIOD_STYLE.get(period.group(1), SYRUP_STYLE) if period else SYRUP_STYLE


colors = {label: style_of(label)[0] for label in order}

fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

sns.lineplot(
    data=df_n,
    x="time",
    y="msg",
    hue="protocol",
    hue_order=order,
    palette=colors,
    style="protocol",
    style_order=order,
    markers=[style_of(label)[1] for label in order],
    # a marker per bin would cover the lines: one per propagation interval, on the beaconing bursts
    markevery=max(1, round(float(args.dprop) / args.bin)),
    alpha=1,
    markersize=8,
    lw=1.5,
)

plt.yscale('symlog', linthresh=0.5)
plt.xlabel('Time [ms]')
plt.ylabel(r'Rate of messages exchanged')

plt.ylim([-0.1, df_n["msg"].max() * 10])  # a decade of headroom on the log axis
plt.xlim([0, xmax])

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

out_path = OUTPUT_DIR / f"msgRate_bcn_{args.network}_{SCENARIO}.pdf"
# no bbox_inches="tight": it crops the figure to its contents, so the pdf would no longer be LINEWIDTH wide
plt.savefig(out_path, format="pdf", dpi=1000)
print(f"saved {out_path}")
plt.show()
