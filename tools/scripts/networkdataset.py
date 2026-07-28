#!/usr/bin/env python3
## File        : Scripts/NetworkDataSets.py
## Description : build datasets, analyze graph properties

import argparse
import numpy as np
from pathlib import Path

import networkx as nx

DEFAULT_AS = "1221"
DEFAULT_TOPDIR = "../../data/topologies/rocketfuel"
DEFAULT_ROCKET_DIR = "../../data/datasets/rocketfuel"
DEFAULT_CLIQUE_DIR = "../../data/datasets/clique"
DEFAULT_TRIANGLE_DIR = "../../data/datasets/triangle"

# ──────────────────────────────────────────────────────────────────────
# Aux functions
# ──────────────────────────────────────────────────────────────────────

_rng: np.random.Generator | None = None


def get_rng() -> np.random.Generator:
    global _rng
    if _rng is None:
        _rng = np.random.default_rng()
    return _rng


def seed_rng(seed: int | None):
    global _rng
    _rng = np.random.default_rng(seed)


def _unit_delay(u: int, v: int) -> int:
    return 1


def _uniform_delay(low=1, high=10):
    return get_rng().uniform(low, high)


def _proportional_length(base: int = 10) -> int:
    return int(np.random.uniform(1, base))


# ──────────────────────────────────────────────────────────────────────
# Rocketfuel loader/dataset writer
# ──────────────────────────────────────────────────────────────────────

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


_DATASET_SPECS = [
    ("DelayLength", "delay + length", ["Delay", "Length"]),
    ("DelayHops", "delay + hop count", ["Delay", "Hops"]),
]


def write_datasets(graph: nx.DiGraph, AS: str, testdir: str):
    out_dir = Path(testdir) / AS
    out_dir.mkdir(parents=True, exist_ok=True)

    for prefix, desc, attrs in _DATASET_SPECS:
        out_file = out_dir / f"{prefix}.tsv"
        print(f"\n\t+ Saving {desc} → '{out_file}'")
        with open(out_file, "w") as f:
            f.write(f"{graph.number_of_nodes()}\n")
            for u, v, data in graph.edges(data=True):
                line = f"{u}\t{v}"
                if "Delay" in attrs:
                    delay = _uniform_delay(0.8 * data['Delay'], data['Delay'])
                    #line += f"\t{delay:.6f}"
                    line += f"\t{data['Delay']}"
                if "Hops" in attrs: line += "\t1"
                if "Length" in attrs: line += f"\t{data['Length']}"
                f.write(line + "\n")


# ──────────────────────────────────────────────────────────────────────
# B-Clique dataset writer
# ──────────────────────────────────────────────────────────────────────
def b_clique_network(
        n: int,
        mode: str = "unit",
        delay_fn=None,
        length_fn=None,
) -> nx.DiGraph:
    if delay_fn is None:
        delay_fn = _uniform_delay if mode == "uniform" else _unit_delay
    if length_fn is None:
        length_fn = _unit_delay

    is_hops = (length_fn is _unit_delay)  # Hops dataset -> length always represents hop count

    g = nx.DiGraph()

    def add(u, v, delay=None, length=None):
        if length is None:
            length = 1 if is_hops else length_fn(u, v)
        if delay is None:
            if mode == "equal":
                delay = length
            else:
                delay = delay_fn(u, v)

        g.add_edge(u, v, Delay=delay, Length=length)
        g.add_edge(v, u, Delay=delay, Length=length)

    # clique edges (nodes 1..n)
    for u in range(1, n + 1):
        for v in range(1, n + 1):
            if u != v:
                add(u, v)

    # special edges, behavior depends on mode
    if mode == "unit":
        add(0, n)  # unit delay, unit/default length
        add(0, 1, delay=1, length=n)  # unit delay, length forced to n
    elif mode == "equal":
        add(0, n, delay=n, length=n)
        add(0, 1, delay=n + 2, length=n + 2)
    elif mode == "uniform":
        add(0, n)  # random delay via delay_fn; length follows is_hops rule
        add(0, 1)
    else:
        raise ValueError(f"Unknown mode: {mode!r}")

    return g


def write_b_clique(n: int, testdir: str, mode: str = "unit"):
    out_dir = Path(testdir) / f"bclique_n{n}"
    out_dir.mkdir(parents=True, exist_ok=True)
    for prefix, desc, attrs in _DATASET_SPECS:
        out_file = out_dir / f"{prefix}_{mode}.tsv"
        print(f"\n\t+ Saving {desc} -> '{out_file}'")

        length_fn = _unit_delay if "Hops" in attrs else _proportional_length
        g = b_clique_network(n, mode=mode, length_fn=length_fn)

        with open(out_file, "w") as f:
            f.write(f"{g.number_of_nodes()}\n")
            for u, v, data in g.edges(data=True):
                line = f"{u}\t{v}"
                line += f"\t{data['Delay']}"
                line += f"\t{data['Length']}"
                f.write(line + "\n")


def b_clique_layout(n: int) -> dict:
    pos = {}
    for i in range(n):
        pos[i] = (i * 1.2, (n - 1 - i) * 0.8)

    half = n // 2
    clique_nodes = list(range(n, 2 * n))

    # top row: first half, left → right
    for j, node in enumerate(clique_nodes[:half]):
        pos[node] = (j * 1.2, 0)

    # bottom row: second half, left → right
    for j, node in enumerate(clique_nodes[half:]):
        pos[node] = (j * 1.2, -1.2)

    return pos


# ──────────────────────────────────────────────────────────────────────
# Triangle network
# ──────────────────────────────────────────────────────────────────────
def triangle_network(max: int, triangledir: str):
    out_dir = Path(triangledir) / f"triangle_n{max}"
    out_dir.mkdir(parents=True, exist_ok=True)

    out_file = out_dir / f"triangle_n{max}.tsv"
    print(f"\n\t+ Saving triangle n={max} -> '{out_file}'")
    with open(out_file, "w") as f:
        f.write(f"{2 * max + 1}\n")
        for x in range(max):
            if 2 * max - x == max + 1:
                f.write(f"{x}\t{x + 1}\t{0}\t{0}\n")
                f.write(f"{x + 1}\t{x}\t{0}\t{0}\n")
                f.write(f"{x}\t{2 * max - x}\t{2 ** (max - (x + 1))}\t{2 ** (max - 1)}\n")
                f.write(f"{2 * max - x}\t{x + 1}\t{0}\t{0}")
                break
            f.write(f"{x}\t{x + 1}\t{2 ** (max - (x + 2))}\t{2 ** (max - (x + 2))}\n")
            f.write(f"{x}\t{2 * max - x}\t{2 ** (max - (x + 1))}\t{2 ** (max - (x + 1))}\n")
            f.write(f"{2 * max - x}\t{x + 1}\t{0}\t{0}\n")


# ──────────────────────────────────────────────────────────────────────
# CLI
# ──────────────────────────────────────────────────────────────────────

def cmd_info(g: nx.DiGraph, AS: str, **_):
    print_graph_info(g, AS)
    u, v, max_d = max(g.edges(data="Delay"), key=lambda e: e[2])
    print(f"\n\tLargest edge delay: {max_d}  on ({u},{v})")


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="NetworkDataSets.py",
        description="Topology toolkit")

    p.add_argument("-a", "--as", dest="AS", default=DEFAULT_AS,
                   help="AS number (default: %(default)s)")

    p.add_argument("--topdir", dest="topdir", default=DEFAULT_TOPDIR)
    p.add_argument("--cliquedir", dest="cliquedir", default=DEFAULT_CLIQUE_DIR)
    p.add_argument("--triangledir", dest="triangledir", default=DEFAULT_TRIANGLE_DIR)
    p.add_argument("--rocketdir", dest="rocketdir", default=DEFAULT_ROCKET_DIR)

    p.add_argument("--seed", type=int, default=None,
                   help="RNG seed for reproducible delay/length sampling "
                        "(default: unseeded, different every run)")

    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("build", help="write TSV dataset files for a Rocketfuel AS")
    sub.add_parser("info", help="print graph statistics for a Rocketfuel AS")

    bc = sub.add_parser("bclique", help="generate a B-Clique synthetic topology")
    bc.add_argument("-n", type=int, default=4, help="B-Clique size")

    bc.add_argument("--mode", choices=["unit", "equal", "uniform"], default="equal",
                    help="delay/length distribution (default: unit)")

    tri = sub.add_parser("triangle", help="print a triangle network to stdout")
    tri.add_argument("-n", type=int, default=6, help="triangle size parameter (default: 6)")

    return p


def main():
    parser = build_parser()
    args = parser.parse_args()
    seed_rng(args.seed)

    if args.cmd == "bclique":
        write_b_clique(args.n, args.cliquedir, mode=args.mode)

    elif args.cmd == "triangle":
        triangle_network(args.n, args.triangledir)

    else:
        # Rocketfuel commands need a graph loaded
        g = load_graph(args.AS, args.topdir)
        dispatch = {
            "build": lambda: write_datasets(g, args.AS, args.rocketdir),
            "info": lambda: cmd_info(g, args.AS),
        }
        dispatch[args.cmd]()


if __name__ == "__main__":
    main()
