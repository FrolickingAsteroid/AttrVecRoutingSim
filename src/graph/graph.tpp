#ifndef _GRAPHS_TPP_
#define _GRAPHS_TPP_

#include <fstream>
#include <stdexcept>

#include "graph.hpp"

namespace Graphs
{
template <class S>
Graph<S>::Graph(int V_)
    : V(V_)
{
	this->E = 0;
	adjListIn.resize(V);
	adjListOut.resize(V);
}

template <class S>
void Graph<S>::addEdge(int u, int v, double delay, S w)
{
	if (0 > u or u >= V)
	{
		throw std::out_of_range("In-Node  does not exist.");
	}
	if (0 > u or v >= V)
	{
		throw std::out_of_range("Out-Node does not exist.");
	}
	E++;
	adjListIn[v][u]  = {delay, true, w};
	adjListOut[u][v] = {delay, true, w};
}

template <class S>
void Graph<S>::removeEdge(int u, int v)
{
	if (u < 0 or u >= V)
	{
		throw std::out_of_range("In-Node does not exist.");
	}
	if (v < 0 or v >= V)
	{
		throw std::out_of_range("Out-Node does not exist.");
	}
	E--;
	adjListIn[v].erase(u);
	adjListOut[u].erase(v);
}

template <class S>
void Graph<S>::activate(int u, int v)
{
	if (0 > u or u >= V)
	{
		throw std::out_of_range("Out-Node  does not exist.");
	}
	if (0 > v or v >= V)
	{
		throw std::out_of_range("In-Node  does not exist.");
	}
	std::get<1>(adjListIn[u].at(v)) = true;
	std::get<1>(adjListIn[v].at(u)) = true;
}

template <class S>
void Graph<S>::deactivate(int u, int v)
{
	if (0 > u or u > V)
	{
		throw std::out_of_range("Out-Node does not exist.");
	}
	if (0 > v or v > V)
	{
		throw std::out_of_range("In-Node  does not exist.");
	}
	std::get<1>(adjListIn[u].at(v)) = false;
	std::get<1>(adjListIn[v].at(u)) = false;
}

template <class S>
void Graph<S>::clear()
{
	V = 0;
	E = 0;
	adjListIn.clear();
	adjListOut.clear();
}

template <class S>
Graph<S> Graph<S>::readGraph(const std::string &filename)
{
	std::ifstream fileNetwork(filename);

	if (!fileNetwork.is_open())
	{
		throw std::runtime_error("Could not open network file: " + filename);
	}

	int V;
	int u;
	int v;

	double delay;
	S      attr;

	fileNetwork >> V;

	Graph<S> graph(V);

	while (fileNetwork >> u >> v >> delay >> attr)
	{
		graph.addEdge(u, v, delay, attr);
	}

	return graph;
}

template <class S>
std::vector<std::pair<int, int>> Graph<S>::findBidLinks()
{
	std::vector<std::pair<int, int>> bidirectional_links;
	for (int u = 0; u < V; ++u)
	{
		for (const auto &[v, _] : adjListOut[u])
		{
			if (u < v && adjListOut[v].count(u))
			{
				bidirectional_links.push_back({u, v});
			}
		}
	}
	return bidirectional_links;
}
} // namespace Graphs
#endif
