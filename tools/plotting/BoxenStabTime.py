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

node_counts = [4, 8, 16, 32, 64]
protocol_paths = {
    "RPBOB": "../../data/output/clique/bclique_n{n}/rpbob/DelayHops_unit_A_hops_N_adaptative_S_linkFailure_period_1.0_convergence.csv",
    "BGP": "../../data/output/clique/bclique_n{n}/bgp/DelayHops_unit_A_hops_S_linkFailure_convergence.csv",
    "RCN": "../../data/output/clique/bclique_n{n}/rcn/DelayHops_unit_A_hops_S_linkFailure_convergence.csv",
}

frames = []
for n in node_counts:
    for protocol, path_template in protocol_paths.items():
        d = pd.read_csv(path_template.format(n=n))
        d["Protocol"] = protocol
        d["n"] = n
        frames.append(d)

df = pd.concat(frames, ignore_index=True)

plt.figure(figsize=(8, 5))

sns.boxplot(data=df, x="n", y="stab_time", hue="Protocol", showmeans=True, meanline=True, meanprops={'color': 'black'})

plt.xlabel('number of nodes')
plt.ylabel('Stabilization Time')
plt.grid(alpha=0.3, axis='y')
plt.tight_layout()
plt.show()
plt.close()
