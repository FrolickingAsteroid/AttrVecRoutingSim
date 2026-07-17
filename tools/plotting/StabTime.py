#!/usr/bin/env python3
import os
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

NODE_COUNTS = [4, 8, 16, 32, 64]
DATASET_TAG = "DelayLength_uniform_A_lengths"
DATA_ROOT = "../../data/output/clique"
OUTPUT_DIR = os.path.join("outputs")

os.makedirs(OUTPUT_DIR, exist_ok=True)


def load_convergence(n):
    base = f"{DATA_ROOT}/bclique_n{n}"
    term_rpbob = pd.read_csv(f"{base}/rpbob/{DATASET_TAG}_N_adaptative_S_linkFailure_period_1.0_convergence.csv")
    term_bgp = pd.read_csv(f"{base}/bgp/{DATASET_TAG}_S_linkFailure_convergence.csv")
    term_rcn = pd.read_csv(f"{base}/rcn/{DATASET_TAG}_S_linkFailure_convergence.csv")

    term_rpbob["Protocol"] = "RPBOB"
    term_bgp["Protocol"] = "BGP"
    term_rcn["Protocol"] = "RCN"

    return pd.concat([term_rpbob, term_bgp, term_rcn], ignore_index=True)


def plot_ccdf(df, n):
    plt.figure(figsize=(6, 4))
    sns.ecdfplot(data=df, x="stab_time", hue="Protocol", complementary=True, lw=1.5)
    plt.xlabel('time [ms]')
    plt.ylabel("CCDF")
    plt.title(f"n = {n}")
    plt.grid(alpha=0.3, which='both')
    plt.tight_layout()

    out_path = os.path.join(OUTPUT_DIR, f"ccdf_n{n}.png")
    plt.savefig(out_path, dpi=300)
    plt.close()
    print(f"saved {out_path}")


for n in NODE_COUNTS:
    try:
        df = load_convergence(n)
    except FileNotFoundError as e:
        print(f"skipping n={n}: {e}")
        continue
    plot_ccdf(df, n)
