#pragma once
#include <random>

enum class Direction { UP, DOWN, LEFT, RIGHT };

inline const char* directions_name(Direction d) {
    switch(d) {
        case Direction::UP:  return "Up";
        case Direction::DOWN:  return "Down";
        case Direction::LEFT:  return "Left";
        case Direction::RIGHT:  return "Right";
    }
    return "?";
}
static constexpr Direction ALL_DIRECTIONS[] = {
    Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT
};

// random number generators
inline std::mt19937& rng() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

// seed the shared engine for reproducible runs (used by the n-tuple trainer)
inline void seed_rng(unsigned s) {
    rng().seed(s);
}

inline int rand_int(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);  // [lo, hi] inclusive
    return dist(rng());
}

inline double rand_double() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);  // [0.0, 1.0)
    return dist(rng());
}