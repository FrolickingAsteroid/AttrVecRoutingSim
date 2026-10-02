#!/usr/bin/env python3
## File        : Scripts/NetworkDataSets.py
## Description : build datasets, analyze graph properties

import argparse
import numpy as np
from pathlib import Path
from itertools import combinations

import networkx as nx

DEFAULT_AS = "1221"
DEFAULT_TOPDIR = "../../data/topologies/rocketfuel"
DEFAULT_ROCKET_DIR = "../../data/datasets/rocketfuel"
DEFAULT_CLIQUE_DIR = "../../data/datasets/clique"
DEFAULT_TRIANGLE_DIR = "../../data/datasets/triangle"
DEFAULT_FOCUS_DIR = "../../data/datasets/focus"
DEFAULT_SCIERA_AS = "sciera"
DEFAULT_SCIERA_TOPDIR = "../../data/topologies/SCION"
DEFAULT_SCIERA_DIR = "../../data/datasets/scion"

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


def _unit_delay() -> int:
    return 1


def _uniform_delay(low=1, high=10):
    return get_rng().uniform(low, high)


def _proportional_length(base: int = 10) -> int:
    return int(np.random.uniform(1, base))


# ──────────────────────────────────────────────────────────────────────
# Rocketfuel loader/dataset writer
# ──────────────────────────────────────────────────────────────────────
_DATASET_SPECS = [
    ("DelayLength", "delay + length", ["Delay", "Length"]),
    ("DelayLengthWidth", "delay + length + width", ["Delay", "Length", "Width"]),
    ("DelayHops", "delay + hop count", ["Delay", "Hops"]),
]


def load_graph(AS: str, topdir: str, source: str = "rocketfuel",
               require_biconnected: bool | None = None) -> tuple[nx.DiGraph, dict]:
    if require_biconnected is None:
        require_biconnected = (source == "rocketfuel")

    if source == "rocketfuel":
        delimiter = None  # whitespace
    elif source == "sciera":
        delimiter = ","
    else:
        raise ValueError(f"Unknown source: {source!r}")

    weight_file = f"{topdir}/{AS}/weights.intra"
    latency_file = f"{topdir}/{AS}/latencies.intra"

    g = nx.Graph()
    with open(weight_file) as fw, open(latency_file) as fl:
        for lw, ll in zip(fw, fl):
            aw = lw.rstrip().split(delimiter)
            al = ll.rstrip().split(delimiter)
            weight = float(aw[2])
            g.add_edge(aw[0], aw[1],
                       Width=int(100 * (1 / weight)) if weight else 0,
                       Length=int(weight),
                       Delay=int(float(al[2])))

    if require_biconnected:
        components = list(nx.biconnected_components(g))
        largest = max(components, key=lambda c: (len(c), sorted(c)))
        dropped = set(g.nodes) - largest
        if dropped:
            print(f"\t! dropping non-biconnected nodes: {sorted(dropped)}")
        g = g.subgraph(largest)

    g = nx.convert_node_labels_to_integers(g, ordering="sorted", label_attribute="orig_name")
    id_to_name = {n: data["orig_name"] for n, data in g.nodes(data=True)}

    return g.to_directed(), id_to_name


def write_node_map(id_to_name: dict, out_dir: str, AS: str):
    out_path = Path(out_dir) / AS / "node_map.tsv"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        f.write("id\tname\n")
        for i in sorted(id_to_name):
            f.write(f"{i}\t{id_to_name[i]}\n")
    print(f"\t+ Saving node id -> name map -> '{out_path}'")


def write_datasets(graph: nx.DiGraph, AS: str, delay: str, testdir: str):
    out_dir = Path(testdir) / AS
    out_dir.mkdir(parents=True, exist_ok=True)

    for prefix, desc, attrs in _DATASET_SPECS:
        if delay == "real":
            out_file = out_dir / f"{prefix}.tsv"
        elif delay == "unit":
            out_file = out_dir / f"{prefix}_unitDelay.tsv"
        elif delay == "equal":
            out_file = out_dir / f"{prefix}_equalDelay.tsv"

        print(f"\n\t+ Saving {desc} → '{out_file}'")
        with open(out_file, "w") as f:
            f.write(f"{graph.number_of_nodes()}\n")
            for u, v, data in graph.edges(data=True):
                line = f"{u}\t{v}"
                if "Delay" in attrs:
                    if delay == "real":
                        line += f"\t{data['Delay']}"
                    elif delay == "unit":
                        line += "\t1"
                    elif delay == "equal":
                        line += f"\t{data['Length']}"

                if "Hops" in attrs: line += "\t1"
                if "Length" in attrs: line += f"\t{data['Length']}"
                if "Width" in attrs: line += f"\t{data['Width']}"
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
            length = 1 if is_hops else length_fn()
        if delay is None:
            if mode == "equal":
                delay = length
            else:
                delay = delay_fn()

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


def write_b_clique(n: int, dir: str, mode: str = "unit"):
    out_dir = Path(dir) / f"bclique_n{n}"
    out_dir.mkdir(parents=True, exist_ok=True)

    for prefix, desc, attrs in _DATASET_SPECS:
        if (mode == "unit" and prefix == "DelayLength") or (mode == "equal" and prefix == "DelayHops"):
            continue

        out_file = out_dir / f"{prefix}_{mode}.tsv"
        print(f"\n\t+ Saving {desc} -> '{out_file}'")

        length_fn = _unit_delay if "Hops" in attrs else _proportional_length
        g = b_clique_network(n, mode=mode, length_fn=length_fn)

        print(nx.diameter(g, weight="Length"))

        with open(out_file, "w") as f:
            f.write(f"{g.number_of_nodes()}\n")
            for u, v, data in g.edges(data=True):
                line = f"{u}\t{v}"
                line += f"\t{data['Delay']}"
                line += f"\t{data['Length']}"
                f.write(line + "\n")


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
# FOCUS network
# ──────────────────────────────────────────────────────────────────────
def focus_network(max: int, focusdir: str):
    out_dir = Path(focusdir) / f"focus_n{max}"
    out_dir.mkdir(parents=True, exist_ok=True)

    out_file = out_dir / f"focus_n{max}.tsv"
    print(f"\n\t+ Saving focus n={max} -> '{out_file}'")
    with open(out_file, "w") as f:
        f.write(f"{max + 2}\n")

        for x in range(max):
            if x == 0:
                f.write(f"{0}\t{x+1}\t{0}\t{2 ** max}\n")
                f.write(f"{x+1}\t{0}\t{0}\t{2 ** max}\n")
                f.write(f"{max + 1}\t{x+1}\t{0}\t{(2 ** max)}\n")
                f.write(f"{x+1}\t{max + 1}\t{0}\t{(2 ** max)}\n")
                continue

            f.write(f"{0}\t{x+1}\t{2 ** x}\t{2 ** (max - x)}\n")
            f.write(f"{x+1}\t{0}\t{2 ** x}\t{2 ** (max - x)}\n")
            f.write(f"{max + 1}\t{x+1}\t{2 ** x}\t{2 ** (max - x)}\n")
            f.write(f"{x+1}\t{max + 1}\t{2 ** x}\t{2 ** (max - x)}\n")

def focus_network_inv(max: int, focusdir: str):
    out_dir = Path(focusdir) / f"focus_n{max}"
    out_dir.mkdir(parents=True, exist_ok=True)

    out_file = out_dir / f"focus_n{max}_inv.tsv"
    print(f"\n\t+ Saving focus n={max} -> '{out_file}'")
    with open(out_file, "w") as f:
        f.write(f"{max + 2}\n")

        for x in range(max):
            if x == 0:
                f.write(f"{0}\t{x+1}\t{2 ** max}\t{2 ** max}\n")
                f.write(f"{x+1}\t{0}\t{2 ** max}\t{2 ** max}\n")
                f.write(f"{max + 1}\t{x+1}\t{(2 ** max)}\t{(2 ** max)}\n")
                f.write(f"{x+1}\t{max + 1}\t{(2 ** max)}\t{(2 ** max)}\n")
                continue

            f.write(f"{0}\t{x+1}\t{2 ** (max - x)}\t{2 ** (max - x)}\n")
            f.write(f"{x+1}\t{0}\t{2 ** (max - x)}\t{2 ** (max - x)}\n")
            f.write(f"{max + 1}\t{x+1}\t{2 ** (max - x)}\t{2 ** (max - x)}\n")
            f.write(f"{x+1}\t{max + 1}\t{2 ** (max - x)}\t{2 ** (max - x)}\n")


# ──────────────────────────────────────────────────────────────────────
# CLI
# ──────────────────────────────────────────────────────────────────────

def print_graph_info(g: nx.DiGraph, AS: str):
    V = g.number_of_nodes()
    E = g.number_of_edges()
    print(f"\n--- Network AS{AS} ---")
    print(f"\tNodes:            {V}")
    print(f"\tEdges:            {E}")
    print(f"\tDistinct lengths: {len(set(d for _, _, d in g.edges.data('Length')))}")
    print(f"\tDistinct delays:  {len(set(d for _, _, d in g.edges.data('Delay')))}")


def edge_centrality(G: nx.DiGraph):
    edge_centrality = nx.edge_betweenness_centrality(G, weight="Length")
    node_centrality = nx.betweenness_centrality(G, weight="Length")
    merged = {}
    for (u, v), score in edge_centrality.items():
        key = frozenset((u, v))
        merged[key] = merged.get(key, 0) + score

    ranked_edges = sorted(merged.items(), key=lambda kv: kv[1], reverse=True)
    ranked_nodes = sorted(node_centrality.items(), key=lambda kv: kv[1], reverse=True)

    top_nodes = ranked_nodes[:10]

    top_k = 10
    candidates = ranked_edges[:top_k]
    print(f"\tLarge Edge centrality:")
    for (u, v), score in candidates:
        print(f"\t\t{u} - {v}: centrality={score:.4f}")

    print(f"\n\tLarge Node centrality:")
    for u, score in top_nodes:
        print(f"\t\t{u}: centrality={score:.4f}")

    ranked_edges = sorted(merged.items(), key=lambda kv: kv[1], reverse=False)
    ranked_nodes = sorted(node_centrality.items(), key=lambda kv: kv[1], reverse=False)

    top_nodes = ranked_nodes[:10]
    candidates = ranked_edges[:top_k]
    print(f"\n\n\tSmall Edge centrality:")
    for (u, v), score in candidates:
        print(f"\t\t{u} - {v}: centrality={score:.4f}")

    print(f"\n\tSmall Node centrality:")
    for u, score in top_nodes:
        print(f"\t\t{u}: centrality={score:.4f}")


def all_minimum_edge_cutsets(G):
    k = nx.edge_connectivity(G)
    edges = list(G.edges())
    cutsets = []
    for combo in combinations(edges, k):
        H = G.copy()
        H.remove_edges_from(combo)
        if not nx.is_connected(H):
            cutsets.append(combo)

    return cutsets


def cmd_info(g: nx.DiGraph, AS: str, **_):
    print_graph_info(g, AS)
    V = g.number_of_nodes()
    max_deg = 0
    max_deg_node = 0
    for node in range(V):
        deg = g.degree[node]
        if deg > max_deg:
            max_deg = deg
            max_deg_node = node

    u, v, max_d = max(g.edges(data="Delay"), key=lambda e: e[2])
    print(f"\n\tLargest edge delay: {max_d}  on ({u},{v})")
    print(f"\tNode with maximum degree: {max_deg_node} with degree {max_deg}\n")

    G = g.to_undirected()
    cutsets = list(nx.all_node_cuts(G))

    k = nx.edge_connectivity(G)
    print(f"\tEdge connectivity: {k}")
    cutsets = all_minimum_edge_cutsets(G)
    print(f"\t{len(cutsets)} minimum cutset(s) of size {k}:")
    for cutset in cutsets:
        print(f"\t\t  {cutset}")

    # edge_centrality(g)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="networkdataset.py",
        description="Network builder tool")

    p.add_argument("--topdir", dest="topdir", default=DEFAULT_TOPDIR, help="default: %(default)s")
    p.add_argument("--rocketdir", dest="rocketdir", default=DEFAULT_ROCKET_DIR, help="default: %(default)s")
    p.add_argument("--cliquedir", dest="cliquedir", default=DEFAULT_CLIQUE_DIR, help="default: %(default)s")
    p.add_argument("--triangledir", dest="triangledir", default=DEFAULT_TRIANGLE_DIR, help="default: %(default)s")
    p.add_argument("--focusdir", dest="focusdir", default=DEFAULT_FOCUS_DIR, help="default: %(default)s")
    p.add_argument("--scieradir", dest="scieradir", default=DEFAULT_SCIERA_DIR, help="default: %(default)s")

    p.add_argument("--seed", type=int, default=None,
                   help="RNG seed for reproducible delay/length "
                        "(default: unseeded, different every run)")

    sub = p.add_subparsers(dest="cmd", required=True)

    # Rocketfuel
    ro = sub.add_parser("rocketfuel", help="generate a Rocketfuel Topology")
    ro.add_argument("--AS", default=DEFAULT_AS,
                    help=f"AS number for the Rocketfuel topology (default: {DEFAULT_AS})")
    ro.add_argument("--mode", choices=["unit", "real", "equal"], default="real", help="")

    ro_sub = ro.add_subparsers(dest="rocketfuel_cmd", required=True)
    ro_sub.add_parser("build", help="write TSV dataset files for a Rocketfuel AS")
    ro_sub.add_parser("info", help="print graph information for a Rocketfuel AS")

    # Bclique
    bc = sub.add_parser("bclique", help="write TSV dataset files for a B-Clique synthetic topology")
    bc.add_argument("-n", type=int, default=4, help="B-Clique size")
    bc.add_argument("--mode", choices=["unit", "equal", "uniform"], default="equal",
                    help="delay/length distribution (default: unit)")

    # SCIERA real-world core topology
    sc = sub.add_parser("sciera", help="generate dataset files from a real-world SCIERA topology")
    sc.add_argument("--AS", default=DEFAULT_SCIERA_AS,
                    help=f"topology label for the SCIERA topology (default: {DEFAULT_SCIERA_AS})")
    sc.add_argument("--topdir", dest="sciera_topdir", default=DEFAULT_SCIERA_TOPDIR,
                    help="directory containing <AS>/weights.intra and <AS>/latencies.intra "
                         "(comma-delimited; default: %(default)s)")
    sc.add_argument("--mode", choices=["unit", "real", "equal"], default="real", help="")
    sc.add_argument("--biconnected", action="store_true",
                    help="drop nodes outside the largest biconnected component "
                         "(off by default -- unlike Rocketfuel, real nodes here "
                         "may legitimately be leaves)")

    sc_sub = sc.add_subparsers(dest="sciera_cmd", required=True)
    sc_sub.add_parser("build", help="write TSV dataset files for the SCIERA topology")
    sc_sub.add_parser("info", help="print graph information for the SCIERA topology")

    # Sequence Triangles
    tri = sub.add_parser("triangle", help="write TSV dataset files for a triangle synthetic topology")
    tri.add_argument("-n", type=int, default=6, help="triangle size parameter (default: 6)")

    foc = sub.add_parser("focus", help="write TSV dataset files for a focus synthetic topology")
    foc.add_argument("-n", type=int, default=3, help="focus size parameter (default: 3)")

    return p


def main():
    parser = build_parser()
    args = parser.parse_args()
    seed_rng(args.seed)

    if args.cmd == "bclique":
        write_b_clique(args.n, args.cliquedir, mode=args.mode)

    elif args.cmd == "triangle":
        triangle_network(args.n, args.triangledir)

    elif args.cmd == "focus":
        focus_network(args.n, args.focusdir)

    elif args.cmd == "sciera":
        print("Loading SCIERA topology")
        g, id_to_name = load_graph(args.AS, args.sciera_topdir, source="sciera",
                                   require_biconnected=args.biconnected)
        dispatch = {
            "build": lambda: (write_datasets(g, args.AS, args.mode, args.scieradir),
                              write_node_map(id_to_name, args.scieradir, args.AS)),
            "info": lambda: cmd_info(g, args.AS),
        }
        dispatch[args.sciera_cmd]()

    else:
        # Rocketfuel dataset commands
        print("Loading Rocketfuel topology")
        g, id_to_name = load_graph(args.AS, args.topdir, source="rocketfuel")
        dispatch = {
            "build": lambda: write_datasets(g, args.AS, args.mode, args.rocketdir),
            "info": lambda: cmd_info(g, args.AS),
        }
        dispatch[args.rocketfuel_cmd]()


if __name__ == "__main__":
    main()
