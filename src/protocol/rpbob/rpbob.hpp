#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <tuple>
#include <vector>

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/seq-window.hpp"
#include "protocol/rpbob/state/path-segment.hpp"

namespace Protocol::Rpbob {
    template<class RA>
    struct RPBOB
            : IProtocol<typename RA::BaseAttr,
                Path<typename RA::Attr>,
                std::variant<Path<typename RA::Attr>, std::pair<int, Path<typename RA::Attr> > > > {
        using Msg = std::variant<Path<typename RA::Attr>, std::pair<int, Path<typename RA::Attr> > >;
        using Base = IProtocol<typename RA::BaseAttr, Path<typename RA::Attr>, Msg>;

        using A = RA::Attr;
        using Ord = RA::Ord;
        using Ext = RA::Ext;

        using Base::graph;
        using Base::cal;
        using Base::data;

        bool cn = false;
        std::vector<SequenceWindow<A, Ord> > s;

        explicit RPBOB(Graph<typename RA::BaseAttr> &graph_, std::size_t N_, bool cn_)
            : Base(graph_), cn(cn_), s(graph_.V, SequenceWindow<A, Ord>(N_)) {
        }

        explicit RPBOB(Graph<typename RA::BaseAttr> &graph_, const std::vector<std::size_t> &Ns, bool cn_)
            : Base(graph_), cn(cn_) {
            s.reserve(graph_.V);

            for (std::size_t n: Ns)
                s.emplace_back(n);
        }

        void MsgReception(int u, int v, Msg msg) override;

        void HandleMsg(int u, int v, Path<A> p);

        void HandleMsg(int u, int v, std::pair<int, Path<A> > notif);

        void DestinationAnnouncement(int t, int n, double period, int max = 1000);

        void LinkFailure(int u, int v) override;

        void process() override;

        std::function<bool(int)> nodeConverged = nullptr;
        std::function<void()> converged = nullptr;

        bool final_state = false;
    };
}

#include "protocol/rpbob/rpbob.tpp"

#endif
