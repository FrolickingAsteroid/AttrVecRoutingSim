#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"

namespace Protocol {
    template<class A, class Ord, class Ext>
    void RPBOB<A, Ord, Ext>::MsgReception(int u, int v, path p) {
        Ext extension;

        path old_opt_attr = s[u].GetBestAttr();
        seq_attr lw(std::get<2>(graph.adjListOut[u].at(v)));// Fetch link attr
        seq_attr candidate = extension(lw, p.getAttr());

        p.addAsEntry(u);
        p.addAttr(candidate);

        if (cal.now() == 11 && u == 68) {
            int summy = 1;
        }

        std::optional<path> changed = s[u].CompareSetAttr(p);

        if (changed.has_value()) {
            data.record(cal.now(), u, s[u].GetBestAttr(), s[u].GetSetAttr());
            if (path new_opt_attr = s[u].GetBestAttr(); old_opt_attr.attribute.a != new_opt_attr.attribute.a) {
                data.record(cal.now(), u, new_opt_attr);
            }

            for (auto &[r, edge]: graph.adjListIn[u]) {
                if (auto &[delay, active, attr] = edge; active && r != v) {
                    this->MsgAdvertisement(
                            r,// receiver
                            u,// sender
                            cal.now() + delay,
                            *changed);
                }
            }
        }
    }

    template<class A, class Ord, class Ext>
    void RPBOB<A, Ord, Ext>::MsgAdvertisement(int u, int v, double delay, path p) {
        // to modify the captured copy inside the lambda
        // use mutable
        cal.schedule(delay, [u, v, p, this]() mutable {
            this->MsgReception(u, v, p);
        });
    }

    template<class A, class Ord, class Ext>
    void RPBOB<A, Ord, Ext>::LinkFailure(int u, int v) {
        graph.deactivate(u, v);
    }

    template<class A, class Ord, class Ext>
    void RPBOB<A, Ord, Ext>::DestinationAnnouncement(int t, int n, double period, int max) {
        path p(t, Ord::Neutral(n));

        for (auto &[r, edge]: graph.adjListIn[t]) {
            if (auto &[delay, active, attr] = edge; active) {
                cal.schedule(cal.now() + delay, [r, t, p, this]() mutable {
                    this->MsgReception(r, t, p);
                });
            }
        }

        // schedule next routing computation

        n += 1;
        double d = period * n;
        if (n < max) {
            cal.schedule(d, [t, n, period, max, this]() mutable {
                this->DestinationAnnouncement(t, n, period, max);
            });
        }
    }
}// namespace Protocol

#endif
