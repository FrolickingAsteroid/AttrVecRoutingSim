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
    for attrs in [["Delay", "Length"], ["Delay", "Hops"]]:
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


def cmd_path(g: nx.DiGraph, src: int, dst: int, weight: str = "Length", **_):
    try:
        length = nx.dijkstra_path_length(g, src, dst, weight=weight)
        path = nx.dijkstra_path(g, src, dst, weight=weight)
        print(f"\n\tShortest {weight} path  {src} → {dst}: {length}")
        print(f"\tPath: {path}")
    except nx.NetworkXNoPath:
        print(f"\tNo path from {src} to {dst}.")


def cmd_worst_path(g: nx.DiGraph, **_):
    all_paths = dict(nx.all_pairs_dijkstra_path(g, weight="Length"))

    best_delay, best_pair, best_path, best_len = -1, None, None, None
    for src, paths in all_paths.items():
        for dst, path in paths.items():
            if src == dst:
                continue
            delay = sum(g[u][v]["Delay"] for u, v in zip(path[:-1], path[1:]))
            length = sum(g[u][v]["Length"] for u, v in zip(path[:-1], path[1:]))
            if delay > best_delay:
                best_delay, best_pair, best_path, best_len = delay, (src, dst), path, length

    print(f"\n\tLargest delay on shortest-Length paths: {best_delay}")
    print(f"\tPair:   {best_pair[0]} → {best_pair[1]}")
    print(f"\tLength: {best_len}")
    print(f"\tPath:   {best_path}")


def cmd_best_failure(g: nx.DiGraph, weight: str = "Length",
                     top_k: int = 5, **_):
    undirected = g.to_undirected()

    edge_count: dict[tuple, int] = {}
    all_paths = dict(nx.all_pairs_dijkstra_path(g, weight=weight))

    for src, paths in all_paths.items():
        for dst, path in paths.items():
            if src == dst:
                continue
            for u, v in zip(path[:-1], path[1:]):
                key = (u, v)
                edge_count[key] = edge_count.get(key, 0) + 1

    bridges = set(nx.bridges(undirected))
    candidates = {
        e: cnt for e, cnt in edge_count.items()
        if e not in bridges
    }

    if not candidates:
        print("\n\tEvery edge is a bridge.")
        return

    ranked = sorted(candidates.items(), key=lambda x: x[1], reverse=True)

    print(f"\n\tTop-{top_k} failure candidates (most paths, non-bridge):\n")
    print(f"\t{'Rank':<6} {'Edge':<14} {'Paths through it':<20} {'Bridge?'}")
    print(f"\t{'-' * 55}")
    for rank, ((u, v), cnt) in enumerate(ranked[:top_k], 1):
        is_bridge = (u, v) in bridges or (v, u) in bridges
        print(f"\t{rank:<6} ({u:>3},{v:>3})    {cnt:<20} {'yes' if is_bridge else 'no'}")


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

    # path — shortest path between two nodes
    sp = sub.add_parser("path", help="shortest path between two nodes")
    sp.add_argument("src", type=int)
    sp.add_argument("dst", type=int)
    sp.add_argument("-w", "--weight", default="Length",
                    choices=["Length", "Delay"],
                    help="edge attribute to minimise (default: Length)")

    # worst-path — largest-delay shortest-length path
    sub.add_parser("worst-path",
                   help="find pair with largest delay on shortest-length path")

    # best-failure — find best link to break
    bf = sub.add_parser("best-failure",
                        help="find top-k failure candidates that keep network connected")
    bf.add_argument("-w", "--weight", default="Length",
                    choices=["Length", "Delay"],
                    help="edge attribute used for shortest paths (default: Length)")
    bf.add_argument("-k", "--top-k", type=int, default=5,
                    help="number of candidates to show (default: 5)")

    return p


def main():
    parser = build_parser()
    args = parser.parse_args()

    g = load_graph(args.AS, args.topdir)

    dispatch = {
        "build": lambda: write_datasets(g, args.AS, args.testdir),
        "info": lambda: cmd_info(g, args.AS),
        "path": lambda: cmd_path(g, args.src, args.dst,
                                 weight=args.weight),
        "worst-path": lambda: cmd_worst_path(g),
        "best-failure": lambda: cmd_best_failure(g,
                                                 weight=args.weight,
                                                 top_k=args.top_k),
    }

    dispatch[args.cmd]()


if __name__ == "__main__":
    main()
