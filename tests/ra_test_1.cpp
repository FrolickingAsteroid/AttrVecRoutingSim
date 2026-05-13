#include "routing-algebras/sequenced-ra.hpp"
#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include <iostream>

using namespace RoutingAlgebras;

void compare(SL a, SL b) {
    int N = 2;
    OSL ord(N);

    if (a == b) {
        std::cout << "Attribute a and b are equal\n";
    } else if (ord(a, b)) {
        std::cout << "Attribute a dominates b\n";
    } else if (ord(b, a)) {
        std::cout << "Attribute b dominates a\n";
    } else {
        std::cout << "Attribute a and b are incomparable\n";
    }
}

void attr(SL &a, SL &b, int a1, int a2, int n1, int n2) {
    a.a = a1;
    b.a = a2;
    a.n = n1;
    b.n = n2;
}

void extend(SL a, SL b) {
    ESL ext;
    SL neut = ESL::neutral;
    std::cout << "neutral = (" << neut.a << ", " << neut.n << ")\n";

    SL result = ext(a, b);
    std::cout << "a = (" << a.a << ", " << a.n << ")\n";
    std::cout << "b = (" << b.a << ", " << b.n << ")\n";
    std::cout << "ext(a, b) = (" << result.a << ", " << result.n << ")\n";
}

int main() {
    SL a;
    SL b;

    // a has smaller a and larger n
    attr(a, b, 1, 2, 2, 0);
    compare(a, b);

    // b has smaller a and larger n
    attr(a, b, 2, 1, 0, 2);
    compare(a, b);

    // a and b have the same attr
    attr(a, b, 1, 1, 2, 2);
    compare(a, b);

    // a has smaller l but b has larger n
    attr(a, b, 1, 2, 1, 2);
    compare(a, b);

    // b has smaller l but a has larger n
    attr(a, b, 2, 1, 2, 1);
    compare(a, b);

    extend(a, b);
    return 0;
}
