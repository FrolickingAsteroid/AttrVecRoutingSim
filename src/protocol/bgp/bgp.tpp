#ifndef BGP_TPP
#define BGP_TPP

#include "protocol/bgp/bgp.hpp"

namespace Protocol::BGP {
    template<class RA>
    void BGP<RA>::process() {
        for (int u = 0; u < graph.V; ++u) {
            auto it = data.finalValue.find(u);
            auto best = s[u].GetElected();

            if (best.attribute == Ord::Null()) {
                continue;
            }

            if (it == data.finalValue.end() || best != it->second) {
                // A msg is sent each time the estimate to the destination change
                this->Send(u, s[u].GetElected());
                data.recordChange(u, cal.now(), best);
            }
        }
    }

    template<class RA>
    void BGP<RA>::LinkFailure(int u, int v) {
        graph.deactivate(u, v);
        data.log = true;
        data.setOffset(cal.now());

        LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

        for (auto [node, neighbor]: {std::pair{u, v}, std::pair{v, u}}) {
            if (s[node].succ == neighbor) {
                s[node].RemoveNeighbor(neighbor);
                if (s[node].ProcessBest()) {
                    this->Send(node, s[node].GetElected());
                    data.recordChange(node, cal.now(), s[node].GetElected());
                }
            }
        }
    }

    template<class RA>
    void BGP<RA>::NodeDown(int t) {
        for (auto &[r, edge]: graph.adjListIn[t]) {
            if (s[r].succ == t) {
                s[r].RemoveNeighbor(t);
                graph.deactivate(r, t);
                if (s[r].ProcessBest()) {
                    this->Send(r, s[r].GetElected());
                    data.recordChange(r, cal.now(), s[r].GetElected());
                }
            }
        }
        data.log = true;
        data.setOffset(cal.now());
    }

    template<class RA>
    void BGP<RA>::DestinationAnnouncement(int t) {
        s[t].SetOrigin(t);
        this->Send(t, s[t].O);
    }

    template<class RA>
    void BGP<RA>::MsgReception(int u, int v, attr received) {
        if (std::get<1>(graph.adjListIn[u].at(v)) == false)
            return;

        attr lw(u, std::get<2>(graph.adjListOut[u].at(v)));
        attr extended = lw + received;
        s[u].UpdateNeighbor(v, extended);
        s[u].ProcessBest(v);
    }
}
#endif
