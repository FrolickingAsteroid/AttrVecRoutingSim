#!/usr/bin/env python3
## File        : Scripts/NetworkDataSets.py
## Description : A script for transforming Rocketfuel topologies into the
##               network data sets that serve as input to the simulator.
import os

import networkx as nx

AS = "1221"
TOPDIR = "../data/topologies/rocketfuel"
TESTDIR = "../data/datasets/rocketfuel"

fileNameWeights = f"{TOPDIR}/{AS}/weights.intra"
fileNameLatencies = f"{TOPDIR}/{AS}/latencies.intra"

graph = nx.Graph()
with open(fileNameWeights) as fileWeight, open(fileNameLatencies) as fileLatencies:
    for lineWeight, lineLatencies in zip(fileWeight, fileLatencies):
        argsWeight = lineWeight.rstrip().split(" ")
        argsLatencies = lineLatencies.rstrip().split(" ")
        node1 = argsWeight[0]
        node2 = argsWeight[1]
        length = int(float(argsWeight[2]))
        delay = int(float(argsLatencies[2]))
        graph.add_edge(node1, node2, Delay=delay, Length=length)

graph = max((graph.subgraph(c) for c in nx.biconnected_components(graph)), key=len)
graph = nx.convert_node_labels_to_integers(graph)
graph = graph.to_directed()

print("\n--- Test Network pertaining to %s ---" % AS)
print("\tNumber of Nodes: %d" % graph.number_of_nodes())
print("\tNumber of Links: %d" % graph.number_of_edges())
print(
    "\tNumber of Distinct Lengths: %d"
    % len(set([length for _, _, length in graph.edges.data("Length")]))
)
print(
    "\tNumber of Distinct Delays: %d"
    % len(set([delay for _, _, delay in graph.edges.data("Delay")]))
)

# ── Build adversarial graph: invert delay so longer edges are faster ────────
delays = [d for _, _, d in graph.edges.data("Delay")]
min_delay = min(delays)
max_delay = max(delays)

adv_graph = nx.DiGraph()
adv_graph.add_nodes_from(graph.nodes(data=True))
for u, v, data in graph.edges(data=True):
    adv_delay = max_delay + min_delay - data["Delay"]
    adv_graph.add_edge(u, v, Delay=adv_delay, Length=data["Length"])

# ── Write both variants ─────────────────────────────────────────────────────
variants = [
    ("", graph, "standard"),
    # ("Adv", adv_graph, "adversarial (delay inverted)"),
]

for prefix, g, desc in variants:
    for Attribute in [
        ["Delay", "Length"],
        ["Delay", "Hops"],
    ]:
        dirNameTestNetwork = f"{TESTDIR}/{AS}"
        fileNameTestNetwork = "%s/%s%s.tsv" % (
            dirNameTestNetwork,
            prefix,
            "".join(Attribute),
        )
        try:
            os.makedirs(dirNameTestNetwork)
        except OSError:
            pass

        print("\n\t+ Saving %s data set to '%s'" % (desc, fileNameTestNetwork))
        with open(fileNameTestNetwork, "w+") as f:
            f.write("%s\n" % str(g.number_of_nodes()))
            for u, v, data in g.edges(data=True):
                line = "%s\t%s" % (str(u), str(v))
                if "Delay" in Attribute:
                    line += "\t%s" % str(data["Delay"])
                if "Hops" in Attribute:
                    line += "\t%s" % "1"
                if "Length" in Attribute:
                    line += "\t%s" % str(data["Length"])
                line += "\n"
                f.write(line)

# ── Diagnostics ─────────────────────────────────────────────────────────────
max_edge = max(graph.edges(data="Delay"), key=lambda e: e[2])
u, v, max_d = max_edge

print(f"\n\tLargest edge delay (standard):    {max_d} on edge ({u},{v})")


def path_weight(g, path, attr):
    return sum(g[u][v][attr] for u, v in zip(path[:-1], path[1:]))


max_delay = -1
max_pair = None
max_path = None
max_length = None

shortest_paths = dict(nx.all_pairs_dijkstra_path(graph, weight="Length"))

for src, paths in shortest_paths.items():
    for dst, path in paths.items():
        if src == dst:
            continue

        delay = path_weight(graph, path, "Delay")
        length = path_weight(graph, path, "Length")

        if delay > max_delay:
            max_delay = delay
            max_pair = (src, dst)
            max_path = path
            max_length = length

print(f"\n\tLargest delay among shortest-Length paths: {max_delay}")
print(f"\tPair: {max_pair[0]} -> {max_pair[1]}")
print(f"\tShortest-path length: {max_length}")
print(f"\tPath: {max_path}")

u = 0
v = 12

dist = nx.dijkstra_path_length(graph, u, v, weight="Length")
path = nx.dijkstra_path(graph, u, v, weight="Length")

print(f"\n\tShortest-path length {u} -> {v}: {dist}")
print(f"\tPath: {path}")
