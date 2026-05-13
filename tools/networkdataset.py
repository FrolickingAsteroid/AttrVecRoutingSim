#!/usr/bin/env python3
## File        : Scripts/NetworkDataSets.py

## Description : A script for transforming Rocketfuel topologies into the
##               network data sets that serve as input to the simulator.

# Public Library
import os

import matplotlib.pyplot as plt
import networkx as nx

## Change to:
##   'for AS in ['1221', '1239', '1755', '3257', '3967', '6461']:
## to generate the test networks pertaining the all the ASs.

AS = "3967"
TOPDIR = "../data/topologies/rocketfuel"
TESTDIR = "../data/datasets/rocketfuel"

fileNameWeights = f"{TOPDIR}/{AS}/weights.intra"
fileNameLatencies = f"{TOPDIR}/{AS}/latencies.intra"

graph = nx.Graph()

with open(fileNameWeights) as fileWeight, open(fileNameLatencies) as fileLatencies:
    pattern = "[0-9]"

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

for Attribute in [
    ["Delay", "Length"],
    ["Delay", "Hops"],
]:
    dirNameTestNetwork = f"{TESTDIR}/{AS}"
    fileNameTestNetwork = "%s/%s.tsv" % (dirNameTestNetwork, "".join(Attribute))

    try:
        os.makedirs(dirNameTestNetwork)
    except OSError as e:
        pass

    fileTestNetwork = open(fileNameTestNetwork, "w+")

    print("\n\t+ Saving Data Set to File '%s' " % fileNameTestNetwork)

    string2file = "%s\n" % str(graph.number_of_nodes())
    fileTestNetwork.write(string2file)

    for u, v, data in graph.edges(data=True):
        string2file = "%s\t%s" % (str(u), str(v))

        if "Delay" in Attribute:
            string2file += "\t%s" % str(data["Delay"])

        if "Hops" in Attribute:
            string2file += "\t%s" % "1"

        if "Length" in Attribute:
            string2file += "\t%s" % str(data["Length"])

        string2file += "\n"

        fileTestNetwork.write(string2file)

    fileTestNetwork.close()

print("")
pos = nx.circular_layout(graph)
nx.draw_networkx(graph, pos)

plt.show()
