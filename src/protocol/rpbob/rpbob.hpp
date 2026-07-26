#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <functional>
#include <tuple>
#include <variant>
#include <vector>
#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/path-segment.hpp"
#include "protocol/rpbob/state/seq-window.hpp"

namespace Protocol::Rpbob
{
    template <class RA>
    struct RPBOB
        : IProtocol<
              typename RA::BaseAttr, Path<typename RA::Attr>,
              std::variant<Path<typename RA::Attr>, std::tuple<int, std::pair<int, int>, Path<typename RA::Attr> > > >
    {
        // Type Aliases
        using BaseAttr = RA::BaseAttr;
        using A        = RA::Attr;
        using Ord      = RA::Ord;
        using Ext      = RA::Ext;

        using PathAttr     = Path<A>;
        using Notification = std::tuple<int, std::pair<int, int>, PathAttr>;
        using Msg          = std::variant<PathAttr, Notification>;

        using Base = IProtocol<BaseAttr, PathAttr, Msg>;

        using Base::cal;
        using Base::data;
        using Base::graph;

        // State
        std::vector<SequenceWindow<A, Ord> > s;

        // Constructors
        explicit RPBOB(Graph<BaseAttr> &graph_, std::size_t N_, bool cn_)
            : Base(graph_),
              s(graph_.V, SequenceWindow<A, Ord>(N_)),
              cn(cn_)
        {}

        explicit RPBOB(Graph<BaseAttr> &graph_, const std::vector<std::size_t> &Ns, bool cn_)
            : Base(graph_),
              cn(cn_)
        {
            s.reserve(graph_.V);
            for (std::size_t n : Ns)
            {
                s.emplace_back(n);
            }
        }

        // Message Processing
        void MsgReception(int u, int v, Msg msg) override;

        void HandleMsg(int u, int v, PathAttr p);

        void HandleMsg(int u, int v, Notification notif);

        void DestinationAnnouncement(int t, int n, double period, int max);

        void LinkFailure(int u, int v) override;

        void process() override;

        // Callbacks
        std::function<bool(int)> nodeConverged = nullptr;
        std::function<void()>    converged     = nullptr;

        bool final_state = false;
        bool cn          = false;
    };
} // namespace Protocol::Rpbob

#include "protocol/rpbob/rpbob.tpp"

#endif // RPBOB_HPP
