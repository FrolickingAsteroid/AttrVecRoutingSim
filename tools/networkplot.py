#!/usr/bin/env python3

import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from pathlib import Path


def read_graph(path: Path):
    with open(path) as f:
        lines = [l.strip() for l in f if l.strip()]
    V = int(lines[0])
    G = nx.DiGraph()
    G.add_nodes_from(range(V))
    for line in lines[1:]:
        u, v, delay, length = map(int, line.split())
        G.add_edge(u, v, delay=delay, length=length)
    return G, V


def plot_graph(G: nx.DiGraph, V: int, intermediate_start: int):
    # separate main chain nodes from intermediates
    main_nodes = [n for n in G.nodes if n < intermediate_start]
    inter_nodes = [n for n in G.nodes if n >= intermediate_start]

    # layout: main chain on a straight horizontal line,
    # intermediates above their corresponding pair
    pos = {}
    for n in main_nodes:
        pos[n] = (n * 10, 0)
    for n in inter_nodes:
        # intermediate i connects i→(i-intermediate_start) to (i-intermediate_start+1)
        i = n - intermediate_start
        pos[n] = (i * 10 + 5, 1.5)

    fig, ax = plt.subplots(figsize=(len(main_nodes) * 2, 6))

    # draw nodes
    nx.draw_networkx_nodes(G, pos, nodelist=main_nodes,
                           node_color="steelblue", node_size=400, ax=ax)
    nx.draw_networkx_nodes(G, pos, nodelist=inter_nodes,
                           node_color="lightgray", node_size=200, ax=ax)
    nx.draw_networkx_labels(G, pos, ax=ax, font_size=7)

    # draw edges with labels
    nx.draw_networkx_edges(G, pos, ax=ax,
                           arrows=True,
                           arrowsize=10)

    edge_labels = {
        (u, v): f"d={d['delay']},l={d['length']}"
        for u, v, d in G.edges(data=True)
    }
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels,
                                 font_size=5, ax=ax)

    # legend
    main_patch = mpatches.Patch(color="steelblue", label="chain nodes")
    inter_patch = mpatches.Patch(color="lightgray", label="intermediate nodes")
    ax.legend(handles=[main_patch, inter_patch], loc="upper right")

    ax.set_title("Graph topology with edge attributes (delay, length)")
    ax.axis("off")
    plt.tight_layout()
    plt.savefig("graph.png", dpi=150, bbox_inches="tight")
    print("Saved: graph.png")


if __name__ == "__main__":
    path = Path("../data/datasets/test/small/50_triangles.tsv")  # your graph file
    intermediate_start = 50  # first intermediate node id
    G, V = read_graph(path)
    plot_graph(G, V, intermediate_start)
