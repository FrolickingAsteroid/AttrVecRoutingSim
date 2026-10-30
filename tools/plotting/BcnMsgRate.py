#!/usr/bin/env python3
"""Rate of messages of RRP against beaconing (normal scenario).

Beaconing nodes all propagate at multiples of dprop, so its messages are sent in bursts: each burst is
drawn as a stem at its instant, its height the messages sent in it. RRP is drawn as a line, binned in BIN ms.
With BIN <= dprop a burst falls in a single bin, so both are in messages per bin.
"""

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
NETWORK = "1755"
SCENARIO = "normal"
DELAY = "DelayLength"
ALGEBRA = "lex-length-hops-suffix"
PERIODS = ["10.0", "70.0"]  # RRP
BCN_PERIOD = "31.0"         # beaconing: destination announcement period
DPROP = "5.0"               # beaconing: propagation interval
BIN = 1  # [ms]
XMAX = 100  # default: a little past the end of the beaconing log

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


parser = argparse.ArgumentParser(description="Plot the rate of messages of RRP against the bursts of beaconing.")
parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
parser.add_argument("-n", "--network", default=NETWORK, help=f"default: {NETWORK}")
parser.add_argument("-d", "--delay", default=DELAY, help=f"default: {DELAY}")
parser.add_argument("-a", "--algebra", default=ALGEBRA, help=f"default: {ALGEBRA}")
parser.add_argument("--periods", nargs="+", default=PERIODS, help=f"RRP periods (default: {' '.join(PERIODS)})")
parser.add_argument("--bperiod", default=BCN_PERIOD, help=f"beaconing announcement period (default: {BCN_PERIOD})")
parser.add_argument("--dprop", default=DPROP, help=f"beaconing propagation interval (default: {DPROP})")
parser.add_argument("--bin", type=float, default=BIN, help=f"RRP bin width [ms] (default: {BIN})")
parser.add_argument("--xmax", type=float, default=XMAX, help="right edge of the x axis")
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


rrp = {}
for period in args.periods:
    df = read_log(net_dir / "rpbob" / SCENARIO / f"{prefix}N_adaptative_S_{SCENARIO}_period_{period}_msglog.csv")
    rrp[period] = bin_series(df.groupby("time")["msg"].sum())

# summed over all destinations, over the time beaconing runs
t_end = bursts.index.max()
print(f"messages in [0, {t_end:g}] ms, summed over destinations")
print(f"  {'Beaconing':<16} total {bursts.sum():>12,.0f}   {bursts.sum() / t_end:>10,.0f} /ms   "
      f"largest burst {bursts.max():>10,.0f} at {bursts.idxmax():g} ms")
for period, binned in rrp.items():
    s = binned[binned.index <= t_end]
    print(f"  {f'RRP Δ = {period}':<16} total {s.sum():>12,.0f}   {s.sum() / t_end:>10,.0f} /ms")

# drawn like MsgRate.py: one line per protocol, binned
rows = [pd.DataFrame({"time": b.index, "msg": b.values,
                      "protocol": f"RRP Δ = {p} ms" if len(rrp) > 1 else "RRP"}) for p, b in rrp.items()]
rows.append(pd.DataFrame({"time": edges, "msg": bin_series(bursts).values, "protocol": "Beaconing"}))
df_n = pd.concat(rows, ignore_index=True)

order = list(df_n["protocol"].unique())
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
    # a marker per bin would cover the lines: one per propagation interval, on the beaconing bursts
    markevery=max(1, round(float(args.dprop) / args.bin)),
    alpha=0.8,
    markersize=8,
    lw=1.5,
)

plt.yscale('symlog', linthresh=0.5)
plt.xlabel('Time [ms]')
plt.ylabel(r'Rate of messages exchanged')

plt.ylim([-0.1, df_n["msg"].max() * 10])  # a decade of headroom on the log axis
plt.xlim([0, XMAX])

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

out_path = OUTPUT_DIR / f"msgRate_bcn_{args.network}_{SCENARIO}.pdf"
plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
print(f"saved {out_path}")
plt.show()
