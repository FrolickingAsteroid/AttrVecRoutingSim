#!/usr/bin/env python3
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

plt.rcParams.update(
    {
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "font.size": 9,
    }
)

file = "DelayHops"
metric = "hops"
mode = "unit"
node_counts = [4, 8, 16, 32, 64]

df_rpbob = pd.read_csv(f'../../data/output/clique/bclique_n64/rpbob/{file}_{mode}_A_{metric}_N_adaptative_S_linkFailure_period_3.0_msglog.csv')
df_bgp = pd.read_csv(f'../../data/output/clique/bclique_n64/bgp/{file}_{mode}_A_{metric}_S_linkFailure_msglog.csv')
df_rcn = pd.read_csv(f"../../data/output/clique/bclique_n64/rcn/{file}_{mode}_A_{metric}_S_linkFailure_msglog.csv")

FAILURE_TIME = 100

def bin_series(df, bin_freq='1s'):
    df = df.copy()
    df['time_ms'] = pd.to_timedelta(df['time'], unit='s')
    series = df.groupby('time_ms')['msg'].sum()
    binned = series.resample(bin_freq).mean()
    binned.index = binned.index.total_seconds().astype(int)
    return binned

binned_rpbob = bin_series(df_rpbob, "9s")
binned_bgp = bin_series(df_bgp, "1s")
binned_rcn = bin_series(df_rcn, "1s")

full_range = range(
    min(binned_rpbob.index.min(), binned_bgp.index.min()),
    max(binned_rpbob.index.max(), binned_bgp.index.max()) + 1,
)
binned_bgp = binned_bgp.reindex(full_range, fill_value=0)
binned_rpbob = binned_rpbob.reindex(full_range, fill_value=0)
binned_rcn = binned_rcn.reindex(full_range, fill_value=0)

df = pd.concat([
    pd.DataFrame({"time": binned_rpbob.index, "msg": binned_rpbob.values, "protocol": r"RPBOB SP $\Delta=1.0$"}),
    pd.DataFrame({"time": binned_rcn.index, "msg": binned_rcn.values, "protocol": "RCN SP"}),
    pd.DataFrame({"time": binned_bgp.index, "msg": binned_bgp.values, "protocol": "BGP SP"}),
], ignore_index=True)

df["time"] = df["time"] - FAILURE_TIME

# ------- Plot -----------------------------#
sns.color_palette("viridis", as_cmap=False)

plt.figure(figsize=(6, 4))

sns.lineplot(data=df, x="time", y="msg", hue="protocol",style="protocol", markers=True, dashes=False, lw=1.5)
plt.yscale('symlog', linthresh=0.5)
plt.xlabel('time [ms]')
plt.ylabel(f'Messages / $\Delta$')

plt.ylim([-0.1, max(binned_bgp.values) + 10**8])
plt.xlim([-0.1, max(df_bgp["time"])- FAILURE_TIME+0.1])
plt.legend()
plt.grid(alpha=0.3, which='both')
plt.tight_layout()
plt.show()
plt.close()
