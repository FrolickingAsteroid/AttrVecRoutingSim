#!/usr/bin/env python3


from pathlib import Path
import seaborn as sns
import pandas as pd

import matplotlib.pyplot as plt
import matplotlib.ticker
import matplotlib

import re
import os

# sized for LaTeX as in https://scipy-cookbook.readthedocs.io/items/Matplotlib_LaTeX_Examples.html:
# the figure is drawn at the width it takes in the paper and included at that width, e.g.
# \includegraphics[width=0.5\linewidth] side by side (or [width=\linewidth] alone), so it isn't scaled
# and these font sizes are the sizes in the paper: 10 pt like the body text, smaller ticks and legend
LINEWIDTH = 345.0  # pt, from LaTeX: \showthe\linewidth

SIDE_BY_SIDE = True
WF, HF = (1, 0.85) if SIDE_BY_SIDE else (1.0, (5. ** 0.5 - 1.0) / 2.0)  # width / LINEWIDTH, height / width

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


matplotlib.rcParams['ytick.minor.size'] = 0
matplotlib.rcParams['ytick.minor.width'] = 0

# ──────────────────────────────────────────────────────────────────────
# Path
# ──────────────────────────────────────────────────────────────────────

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = {
    "clique": "bclique",
    "triangle": "triangle",
}

DATASET_ = "clique"
SIZE = 32                                        # the single clique to plot
SCENARIO = "linkFailure"                         # subfolder inside rpbob/

PERIODS = ["4.0", "8.0", "16.0", "32.0"]  # x axis
PROTOCOLS = ["SyRuP", "SyRuP-LFN"]               # one line each

ESTIMATOR = "max"                                # "max" or "mean"
YTICKS = [4, 8, 16, 32, 64]

FAMILY_STYLE = {                                 # line style, marker
    "BGP": ("-", "s"),
    "BGP-RCN": ("-.", "o"),
    "SyRuP": ("--", "v"),
    "SyRuP-LFN": (":", "D"),
}
families = ["BGP", "BGP-RCN", "SyRuP", "SyRuP-LFN"]

DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET_
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "bgp": "BGP",
    "rpbob": "SyRuP",
    "rcn": "BGP-RCN",
}


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob":
        return "SyRuP-LFN" if "notif" in filename else "SyRuP"
    return PROTOCOL.get(key, protocol_raw.upper())


def fmt_period(period: str) -> str:
    return f"{float(period):g}"  # "16.0" -> "16"


SIZE_DIR = DATA_DIR / f"{DATASET[DATASET_]}_n{SIZE}"

rows = []
# layout: bclique_n32 / rpbob / linkFailure / *_convergence.csv
for csv_path in SIZE_DIR.glob(f"rpbob/*convergence.csv"):
    protocol = csv_path.parent.name
    filename = csv_path.name.lower()

    if "altered" in filename:
        continue

    match = re.search(r"period_(\d+\.\d+)", filename)
    if not match:
        continue
    delta = match.group(1)
    if delta not in PERIODS:
        continue

    protocol_label = get_protocol_label(protocol, filename)
    if protocol_label not in PROTOCOLS:
        continue

    df = pd.read_csv(csv_path)
    df = df[df["change_count"] != 0]

    df = df[['stab_time']].copy()
    df['period'] = fmt_period(delta)
    df['protocol'] = protocol_label
    rows.append(df)

if not rows:
    raise SystemExit(f"no data found in {SIZE_DIR / 'rpbob'}")

data = pd.concat(rows, ignore_index=True)

hue_order = [p for p in families if p in data["protocol"].unique()]
x_order = [fmt_period(p) for p in PERIODS if fmt_period(p) in data["period"].unique()]

# same colours as in the size plot, so SyRuP / SyRuP-LFN look the same across figures
family_colors = dict(zip(families, sns.color_palette("colorblind", n_colors=len(families))))
palette = {p: family_colors[p] for p in hue_order}

fig, ax = plt.subplots(figsize=get_figsize(LINEWIDTH, wf=WF, hf=HF))

print(data.groupby(["period", "protocol"])["stab_time"].agg(ESTIMATOR).unstack().reindex(x_order))

sns.pointplot(
    data=data,
    x="period",
    y="stab_time",
    hue="protocol",
    order=x_order,
    hue_order=hue_order,
    palette=palette,
    estimator="max",
    errorbar=None,
    markers=[FAMILY_STYLE[p][1] for p in hue_order],
    linestyle=[FAMILY_STYLE[p][0] for p in hue_order],
    dodge=0.2,
    markersize=8,
    linewidth=1,
    ax=ax,
)

ax.set_yscale('log')

ax.set_xlabel("Period Δ [ms]")
ax.set_ylabel("Stabilization time [ms]")

ax.set_yticks(YTICKS)
ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
ax.grid(alpha=0.3, which="major")

legend = ax.legend(title=None, loc="upper left")
for text in legend.get_texts():
    text.set_fontfamily("monospace")

plt.tight_layout()  # fits the labels inside the figure, which keeps its size

name = f"{DATASET_}{SIZE}_{SCENARIO}_PeriodPointPlot.pdf"
out_path = os.path.join(OUTPUT_DIR, name)

plt.savefig(out_path, format="pdf")
print(f"\tsaved {out_path}\n")
plt.show()