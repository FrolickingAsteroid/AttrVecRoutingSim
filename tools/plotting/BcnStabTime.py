#!/usr/bin/env python3
"""CCDF of the stabilisation time of beaconing against RRP and BGP (normal scenario).

Beaconing never settles on a single path, so two times are shown per (destination, node):
  - first path: the first beacon the node receives (from the progress log);
  - path set:   the last change of its best set, i.e. when it holds its final set of paths
                (stab_time of the convergence log).
For RRP and BGP the stabilisation time is the stab_time of their convergence logs.

The progress log is too large to load (tens of GB): it is streamed once and the first-path times are
cached next to it in *_firstpath.csv.
"""

import matplotlib.pyplot as plt
from pathlib import Path
import seaborn as sns
import pandas as pd
import numpy as np
import argparse
import pickle

plt.rcParams.update({
    "font.family": "Arial",
    "mathtext.fontset": "cm",
    "font.size": 11,
    "axes.labelsize": 11,
    "xtick.labelsize": 11,
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
PROTOCOLS = ["BGP", "RRP", "Beaconing"]
PERIOD = "10.0"             # RRP: in the normal scenario every period gives the same curve
BCN_PERIOD = "31.0"         # beaconing: destination announcement period
DPROP = "5.0"               # beaconing: propagation interval


LOGY = False

DATA_DIR = ROOT_DIR / 'data' / 'output'
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

FAMILIES = ["BGP", "BGP-RCN", "RRP", "RRP-N", "Beaconing"]


# Source - https://stackoverflow.com/a/31527287
# Posted by rth, modified by community. See post 'Timeline' for change history
# Retrieved 2026-09-27, License - CC BY-SA 4.0

def get_figsize(columnwidth, wf=0.5, hf=(5. ** 0.5 - 1.0) / 2.0, ):
    fig_width_pt = columnwidth * wf
    inches_per_pt = 1.0 / 72.27  # Convert pt to inch
    fig_width = fig_width_pt * inches_per_pt  # width in inches
    fig_height = fig_width * hf  # height in inches
    return [fig_width, fig_height]


parser = argparse.ArgumentParser(description="Plot the CCDF of the stabilisation time of beaconing.")
parser.add_argument("-t", "--topo", default=DATASET, help=f"default: {DATASET}")
parser.add_argument("-n", "--network", default=NETWORK, help=f"default: {NETWORK}")
parser.add_argument("-d", "--delay", default=DELAY, help=f"default: {DELAY}")
parser.add_argument("-a", "--algebra", default=ALGEBRA, help=f"default: {ALGEBRA}")
parser.add_argument("-p", "--protocols", nargs="+", default=PROTOCOLS, help=f"default: {' '.join(PROTOCOLS)}")
parser.add_argument("--period", default=PERIOD, help=f"RRP period (default: {PERIOD})")
parser.add_argument("--bperiod", default=BCN_PERIOD, help=f"beaconing announcement period (default: {BCN_PERIOD})")
parser.add_argument("--dprop", default=DPROP, help=f"beaconing propagation interval (default: {DPROP})")
parser.add_argument("--logy", action="store_true", default=LOGY, help="log y axis")
args = parser.parse_args()

net_dir = DATA_DIR / args.topo / args.network
prefix = f"{args.delay}_A_{args.algebra}_"


def check(path):
    if not path.exists() or path.stat().st_size == 0:  # interrupted runs leave empty files behind
        raise SystemExit(f"nothing at {path}")
    return path


def stab_times(path):
    df = pd.read_csv(check(path), usecols=["dest", "node", "stab_time", "change_count"])
    return df.loc[(df["change_count"] != 0) & (df["node"] != df["dest"]), "stab_time"].dropna()


def first_path_times(progress):
    cache = progress.with_name(progress.name.replace("_progress.csv", "_firstpath.csv"))
    if not cache.exists() or cache.stat().st_mtime < progress.stat().st_mtime:
        print(f"streaming {progress.name} (once, cached in {cache.name})")
        first = {}
        with open(check(progress), "rb") as f:
            next(f)  # header: dest,node,time,opt,wind,tag
            for line in f:
                dest, node, time, _ = line.split(b",", 3)
                if (dest, node) not in first and line.rstrip().endswith(b",reception"):
                    first[dest, node] = float(time)
        pd.DataFrame([(int(d), int(n), t) for (d, n), t in first.items()],
                     columns=["dest", "node", "time"]).to_csv(cache, index=False)
    df = pd.read_csv(cache)
    return df.loc[df["node"] != df["dest"], "time"]


def ccdf(data):
    x = np.concatenate([[0.0], np.sort(data)])
    y = np.concatenate([[1.0], 1 - np.arange(1, len(data) + 1) / len(data)])
    return x, y


# tick label as a module-level function: the figure is pickled, and lambdas can't be
def decimal_label(v, _):
    decimals = max(0, -int(np.floor(np.log10(v))))  # 1, 0.1, 0.01, ... never 1e-05
    return f"{v:.{decimals}f}"


colors = dict(zip(FAMILIES, sns.color_palette("colorblind", n_colors=len(FAMILIES))))

# (label, family, line style, times)
curves = []
if "BGP" in args.protocols:
    curves.append(("BGP", "BGP", "-", stab_times(net_dir / "bgp" / SCENARIO / f"{prefix}S_{SCENARIO}_convergence.csv")))
if "RRP" in args.protocols:
    path = net_dir / "rpbob" / SCENARIO / f"{prefix}N_adaptative_S_{SCENARIO}_period_{args.period}_convergence.csv"
    curves.append(("RRP", "RRP", "-", stab_times(path)))
if "Beaconing" in args.protocols:
    base = net_dir / "beaconing" / SCENARIO / f"{prefix}S_{SCENARIO}_dprop_{args.dprop}_period_{args.bperiod}_"
    curves.append(("Beaconing, first path", "Beaconing", (0, (5, 2)),
                   first_path_times(Path(f"{base}progress.csv"))))
    curves.append(("Beaconing, path set", "Beaconing", "-", stab_times(Path(f"{base}convergence.csv"))))

print(pd.DataFrame({label: t.describe() for label, _, _, t in curves}).T.round(2).to_string())

columnwidth = 345.0
fig, ax = plt.subplots(figsize=get_figsize(columnwidth, wf=1.1, hf=0.7))

width = max(len(label) for label, _, _, _ in curves)
ax.plot([], [], ls="none", label=f"{'':<{width}} | mean")  # header row, aligned with the means
for label, family, ls, t in curves:
    x, y = ccdf(t.to_numpy())
    ax.step(x, y, where="post", color=colors[family], ls=ls, lw=1.5,
            label=f"{label:<{width}} | {t.mean():4.1f} ms")

if args.logy:
    ax.set_yscale("log")
    ax.set_ylim(bottom=1 / (10 * max(len(t) for _, _, _, t in curves)))
    ax.yaxis.set_major_formatter(plt.FuncFormatter(decimal_label))
else:
    ax.set_ylim(0, 1.02)
ax.set_xlim(left=0)
ax.set_xlabel("Time [ms]")
ax.set_ylabel("CCDF")

legend = ax.legend(loc="lower left" if args.logy else "upper right", fancybox=True, shadow=False, fontsize=8)
for text in legend.get_texts():
    text.set_fontfamily("monospace")

ax.grid(alpha=0.3, which='both')
plt.tight_layout()

suffix = "_logy" if args.logy else ""
out_path = OUTPUT_DIR / f"ccdf_bcn_{args.network}_{SCENARIO}{suffix}.pdf"
plt.savefig(out_path, format="pdf", bbox_inches="tight", dpi=1000)
pickle.dump(fig, open(f'raw_plots/ccdf_bcn_{args.network}_{SCENARIO}{suffix}', 'wb'))
print(f"saved {out_path}")
plt.show()
