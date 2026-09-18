#pragma once
#include "../core/game2048.h"
#include "../utils/helpers.h"
#include <unordered_map>
#include <cstdint>
#include <chrono>

class NTupleNetwork;

class ExpectimaxAI {
public:
    explicit ExpectimaxAI(int max_depth);
    Direction get_move(const Game2048& game);

    // Optional: evaluate leaves with a learned n-tuple value function instead
    // of the hand-written heuristic. Search then compounds a far better leaf
    // estimate, which is how the strongest published 2048 results are built.
    // Pass nullptr to go back to Heuristics::evaluate.
    void set_value_network(const NTupleNetwork* net) { _value_net = net; }
    bool uses_value_network() const { return _value_net != nullptr; }

    // stats from last get_move() call
    int last_search_depth = 0;
    int last_nodes_evaluated = 0;
    double last_search_time = 0.0;

private:
    int max_depth;
    const NTupleNetwork* _value_net = nullptr;

    struct CacheEntry {  // transposition table
        double value;
        int depth;
        uint64_t board;  // verified on hit: cache_key() can collide
    };
    std::unordered_map<uint64_t, CacheEntry> cache;
    int nodes_evaluated;

    double expectimax(Game2048& game, int depth, bool is_max, double probability);
    double max_node(Game2048& game, int depth, double probability);
    double chance_node(Game2048& game, int depth, double probability);

    // fewer empty cells -> more critical position -> search deeper
    int adaptive_depth(const Game2048& game) const;

    // prune branches whose cumulative probability is below this. At 0.001 the
    // search died after ~3 chance levels regardless of the requested depth.
    static constexpr double PROBABILITY_THRESHOLD = 0.0001;

    static uint64_t cache_key(uint64_t board_hash, int depth, bool is_max);
};