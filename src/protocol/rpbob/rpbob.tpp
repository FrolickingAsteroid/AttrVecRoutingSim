#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"

using namespace RoutingAlgebras;

namespace Protocol::Rpbob {
    template<class RA>
    void RPBOB<RA>::process() {
        for (int u = 0; u < graph.V; ++u) {
            auto it = data.finalValue.find(u);
            Path<A> best = s[u].GetBestAttr();

            if (best.attr == Ord::Null()) {
                continue;
            }

            if (it == data.finalValue.end() || best.attr.a != it->second.attr.a) {
                data.recordChange(u, cal.now(), best);
            }
        }
    }

    template<class RA>
    void RPBOB<RA>::LinkFailure(int u, int v) {
        data.log = true;
        data.setOffset(cal.now());

        graph.deactivate(u, v);

        LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

        if (cn == true) {
            for (auto [node, link]: {std::tuple{u, std::pair{u, v}}, std::tuple{v, std::pair{v, u}}}) {
                auto removed = s[node].FailureNotif(link);
                if (!removed.empty()) {
                    for (const auto &current: removed) {
                        auto notif = std::make_pair(current.getAttr().n, current);
                        this->Send(node, Msg{notif});
                    }
                }
            }
        }
    }

    template<class RA>
    void RPBOB<RA>::MsgReception(int u, int v, Msg msg) {
        std::visit([&](auto &&m) { this->HandleMsg(u, v, m); }, msg);
    }

    template<class RA>
    void RPBOB<RA>::HandleMsg(int u, int v, std::pair<int, Path<A> > notif) {
        (void) v;

        notif.second.addAsEntry(u);

        if (auto removed = s[u].FailureNotif(notif.second); !removed.empty()) {
            for (const auto &current: removed) {
                auto notif_ = std::make_pair(current.getAttr().n, current);
                this->Send(u, Msg{notif_});
            }
        }
    }

    template<class RA>
    void RPBOB<RA>::HandleMsg(int u, int v, Path<A> p) {
        if (std::get<1>(graph.adjListIn[u].at(v)) == false)
            return;

        Ext extension;

        A lw(std::get<2>(graph.adjListOut[u].at(v))); // Fetch link attr
        A candidate = extension(lw, p.getAttr());

        p.addAsEntry(u);
        p.addAttr(candidate);

        std::optional<Path<A> > changed = s[u].CompareSetAttr(p);

        if (changed.has_value()) {
            this->Send(u, Msg{p});
            converged();
        }
    }

    template<class RA>
    void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, int max) {
        s[t].SetOrigin(n, t);
        auto o = s[t].GetBestAttr();

        this->Send(t, Msg{o});

        n += 1;

        double d = period * n;
        if (final_state == false || n < max)
            cal.schedule(d, [t, n, period, max, this]() mutable {
                this->DestinationAnnouncement(t, n, period, max);
            });
    }
}

#endif
