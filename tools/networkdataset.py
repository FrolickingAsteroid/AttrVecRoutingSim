#!/usr/bin/env python3
## File        : Scripts/NetworkDataSets.py
## Description : build datasets, analyse graph properties,
#                and find good candidate links to fail.

import argparse
import os
import sys
from pathlib import Path

import networkx as nx

DEFAULT_AS = "1221"
DEFAULT_TOPDIR = "../data/topologies/rocketfuel"
DEFAULT_TESTDIR = "../data/datasets/rocketfuel"


def load_graph(AS: str, topdir: str) -> nx.DiGraph:
    weight_file = f"{topdir}/{AS}/weights.intra"
    latency_file = f"{topdir}/{AS}/latencies.intra"

    g = nx.Graph()
    with open(weight_file) as fw, open(latency_file) as fl:
        for lw, ll in zip(fw, fl):
            aw = lw.rstrip().split()
            al = ll.rstrip().split()
            g.add_edge(aw[0], aw[1],
                       Length=int(float(aw[2])),
                       Delay=int(float(al[2])))

    g = max((g.subgraph(c) for c in nx.biconnected_components(g)), key=len)
    g = nx.convert_node_labels_to_integers(g)
    return g.to_directed()


def print_graph_info(g: nx.DiGraph, AS: str):
    print(f"\n--- Network AS{AS} ---")
    print(f"\tNodes:            {g.number_of_nodes()}")
    print(f"\tEdges:            {g.number_of_edges()}")
    print(f"\tDistinct lengths: {len(set(d for _, _, d in g.edges.data('Length')))}")
    print(f"\tDistinct delays:  {len(set(d for _, _, d in g.edges.data('Delay')))}")


def write_datasets(graph: nx.DiGraph, AS: str, testdir: str):
    for attrs in [["Delay", "Length"], ["Delay", "Hops"], ["Delay", "Hops"]]:
        out_dir = Path(testdir) / AS
    out_file = out_dir / f"{prefix}{''.join(attrs)}.tsv"
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"\n\t+ Saving {desc} → '{out_file}'")
    with open(out_file, "w") as f:
        f.write(f"{graph.number_of_nodes()}\n")
    for u, v, data in graph.edges(data=True):
        line = f"{u}\t{v}"
    if "Delay" in attrs: line += f"\t{data['Delay']}"
    if "Hops" in attrs: line += "\t1"
    if "Length" in attrs: line += f"\t{data['Length']}"
    f.write(line + "\n")


def cmd_info(g: nx.DiGraph, AS: str, **_):
    print_graph_info(g, AS)

    u, v, max_d = max(g.edges(data="Delay"), key=lambda e: e[2])
    print(f"\n\tLargest edge delay: {max_d}  on ({u},{v})")


def triangle_network(max: int):
    print(2 * max + 1)

    for x in range(max):
        print(f"{x}\t{x + 1}\t{max - 1 - x}\t{max - 1 - x}")

        if (2 * max - x == max + 1):
            print(f"{x}\t{2 * max - x}\t{2 ** (max - (x + 1))}\t{10}")
            print(f"{2 * max - x}\t{x + 1}\t{0}\t{0}")
            break

        print(f"{x}\t{2 * max - x}\t{2 ** (max - (x + 1))}\t{2 ** (max - (x + 1))}")
        print(f"{2 * max - x}\t{x + 1}\t{0}\t{0}")


# ─── CLI ──────────────────────────────────────────────────────────────────────

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="NetworkDataSets.py",
        description="Rocketfuel topology toolkit")

    p.add_argument("-a", "--as", dest="AS", default=DEFAULT_AS,
                   help="AS number (default: %(default)s)")
    p.add_argument("--topdir", dest="topdir", default=DEFAULT_TOPDIR)
    p.add_argument("--testdir", dest="testdir", default=DEFAULT_TESTDIR)

    sub = p.add_subparsers(dest="cmd", required=True)

    # build — write dataset files
    sub.add_parser("build", help="write TSV dataset files")

    # info — print graph statistics
    sub.add_parser("info", help="print graph statistics")

    return p


def main():
    #triangle_network(30)
    parser = build_parser()
    args = parser.parse_args()

    g = load_graph(args.AS, args.topdir)

    dispatch = {
        "build": lambda: write_datasets(g, args.AS, args.testdir),
        "info": lambda: cmd_info(g, args.AS)
    }

    dispatch[args.cmd]()


if __name__ == "__main__":
    main()
