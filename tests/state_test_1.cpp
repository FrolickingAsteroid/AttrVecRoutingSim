#include "protocol/state/seq-window.hpp"
#include "routing-algebras/base-attributes/orders/lengths-ord.hpp"
#include "routing-algebras/sequenced-ord.hpp"
#include "routing-algebras/sequenced-ext.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace ProtocolState;
using namespace RoutingAlgebras;

static ASEntry MakeEntry(uint64_t cur, uint64_t next, uint64_t in, uint64_t out) {
    ASEntry e;
    e.isd_as = cur;
    e.next_isd_as = next;
    e.hop_entry.ingress = in;
    e.hop_entry.egress = out;
    return e;
}

static void TestNullAttr() {
    Path<int, ESL> p;
    p.attribute = OSL::null;

    std::cout << "Null attr test:\n (a: " << p.attribute.a
            << ", n: " << p.attribute.n << ")\n\n";
}

static void TestPathPrinting() {
    Path<int, ESL> p;
    p.attribute = {3, 10};

    p.addAsEntry(MakeEntry(10, 20, 1, 0));
    p.addAsEntry(MakeEntry(20, 30, 3, 2));
    p.addAsEntry(MakeEntry(30, 0, 0, 4));

    std::ostringstream oss;
    oss << p;

    std::cout << "Printed path:\n" << oss.str() << "\n";
    assert(!oss.str().empty());
}

static void TestSeqOrder() {
    OSL ord(2);

    SL a{5, 10};
    SL b{2, 20};
    SL c{5, 10};

    assert(a == c);
    assert(ord(a, b) == 1);
    assert(ord(c, a) == 0);

    std::cout << "SeqOrd test passed\n\n";
}

static void TestWindowConstruction() {
    Path<int, ESL> p;
    p.attribute = {1, 3};
    p.addAsEntry(MakeEntry(10, 20, 1, 2));

    SequenceWindow<int, ESL, OSL> sw(3, p);

    p.attribute = {2, 3};
    p.addAsEntry(MakeEntry(10, 20, 1, 2));

    sw.CompareSetAttr(p);

    p.attribute = {3, 4};
    p.addAsEntry(MakeEntry(10, 20, 1, 2));

    sw.CompareSetAttr(p);

    p.attribute = {6, 5};
    p.addAsEntry(MakeEntry(10, 20, 1, 2));

    sw.CompareSetAttr(p);
    std::ostringstream oss;
    oss << sw;
    std::cout << "SequenceWindow:\n" << oss.str() << "\n";
}

int main() {
    TestNullAttr();
    TestPathPrinting();
    TestSeqOrder();
    TestWindowConstruction();

    std::cout << "All tests finished\n";
    return 0;
}
