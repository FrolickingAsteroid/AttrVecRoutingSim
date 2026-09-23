#ifndef SEQ_EXTENSION_H
#define SEQ_EXTENSION_H

#include "routing-algebras/attributes/sequenced-attributes.hpp"

namespace RoutingAlgebras
{
template <class A, class ExtBase>
struct SeqExtension
{
	SeqAttr<A> operator()(const SeqAttr<A> &l, const SeqAttr<A> &r) const
	{
		ExtBase ext;
		// Keep the sequence number from the new attr
		// and extend base attribute
		SeqAttr<A> result(l.getSeqNum(), ext(l.getAttr(), r.getAttr()));
		return result;
	}
};
} // namespace RoutingAlgebras

#endif
