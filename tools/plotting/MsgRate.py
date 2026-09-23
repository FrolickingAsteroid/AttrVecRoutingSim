#!/usr/bin/env python3

import matplotlib.pyplot as plt
from pathlib import Path
import seaborn as sns
import pandas as pd
import re

plt.rcParams.update({
    "font.family": "Liberation Sans",
    "mathtext.fontset": "cm",
    "font.size": 10,
})

ROOT_DIR = next(
    p for p in Path(__file__).parents
    if (p / '.git').exists()
)

DATASET = "clique"
DATASET_ = "bclique"
DATA_DIR = ROOT_DIR / 'data' / 'output' / DATASET
OUTPUT_DIR = ROOT_DIR / 'data' / "plots"

PROTOCOL = {
    "rpbob": "RRP",
    "bgp": "BGP",
    "rcn": "BGP-RCN",
}


def get_protocol_label(protocol_raw: str, filename: str) -> str:
    key = protocol_raw.lower()
    if key == "rpbob" and "notif" in filename.lower():
        return "RRP-N"
    return PROTOCOL.get(key, protocol_raw.upper())


def bin_series(df, bin_freq='10s', bin=10):
    df = df.copy()
    df['time_ms'] = pd.to_timedelta(df['time'], unit='s')
    series = df.groupby('time_ms')['msg'].sum()
    binned = series.resample(bin_freq).sum()

    binned.index = binned.index.total_seconds().astype(int)
    return binned


rows = []
raw_frames = {}

for n in [32]:
    for csv_path in DATA_DIR.glob(f"{DATASET_}_n{n}/*/*linkFailure*msglog.csv"):
        protocol = csv_path.parent.name
        filename = csv_path.name
        protocol_label = get_protocol_label(protocol, csv_path.name)

        # change this
        if protocol.lower() == "rpbob" and re.findall(r"\d+\.\d+", filename)[0] != "1.0":
            continue

        df_raw = pd.read_csv(csv_path)[['time', 'msg']]
        #df_raw = df_raw[df_raw["time"] >= 99]

        # if "RRP" in protocol_label:
        #     df_ = pd.read_csv(
        #         f"{DATA_DIR}/{DATASET_}_n{n}/{protocol}/{filename[:filename.rfind("msglog")]}convergence.csv")
        #     max_time = df_["stab_time"].max() + 100
        #     df_raw = df_raw[df_raw["time"] <= max_time]

        raw_frames[(protocol_label, n)] = df_raw

# bgp_raw = pd.concat(
#     df for (label, n), df in raw_frames.items() if label == "BGP"
# )

# max_time = bgp_raw["time"].max() + 10

for (protocol_label, n), df_raw in raw_frames.items():
    # if df_raw["time"].max() < max_time:
    #     pad_row = pd.DataFrame({"time": [max_time], "msg": [0]})
    #     df_raw = pd.concat([df_raw, pad_row], ignore_index=True)

    binned = bin_series(df_raw)

    rows.append(pd.DataFrame({
        "time": binned.index,
        "msg": binned.values,
        "protocol": protocol_label,
        "dataset": n,
    }))

clique_data = pd.concat(rows, ignore_index=True)

for n in [32]:
    df_n = clique_data[clique_data["dataset"] == n].copy()

    if df_n.empty:
        print(f"no data for n={n}, skipping")
        continue

    sns.color_palette("colorblind", as_cmap=False)

    fig, ax = plt.subplots(figsize=(6, 4))

    sns.lineplot(
        data=df_n,
        x="time",
        y="msg",
        hue="protocol",
        hue_order=["RRP", "RRP-N", "BGP", "BGP-RCN"],
        style="protocol",
        style_order=["RRP", "RRP-N", "BGP", "BGP-RCN"],
        markers=['s', "o", "v", "D"],
        # markers=None,
        alpha=0.9,
        markersize=8,
        lw=1.5,
    )
    plt.yscale('symlog', linthresh=0.5)
    plt.xlabel('Time [ms]')
    plt.ylabel(r'Rate of messages exchanged')

    plt.ylim([-0.1, df_n["msg"].max() + 10 ** 6])
    plt.xlim([100, clique_data["time"].max()])

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, title="Protocol", frameon=True, loc="lower right")
    legend = ax.legend_
    legend.get_title().set_fontweight("bold")

    plt.grid(alpha=0.3, which='both')
    plt.tight_layout()
    plt.show()

    # out_path = OUTPUT_DIR / f"msgRate_n{n}.png"
    # plt.savefig(out_path, dpi=300)
    # plt.close()
    # print(f"saved {out_path}")
