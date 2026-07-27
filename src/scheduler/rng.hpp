#ifndef RNG_HPP
#define RNG_HPP

#include <random>

inline std::mt19937 rng_engine;

inline void rng_seed(const unsigned int seed) {
    rng_engine.seed(seed);
}

inline double rng_uniform(double a, double b) {
    if (b < a) std::swap(a, b);
    std::uniform_real_distribution dist(a, b);
    return dist(rng_engine);
}
#endif
