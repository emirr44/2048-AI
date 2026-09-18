#pragma once
#include <string>

#include "../core/game2048.h"
#include "../utils/helpers.h"
#include "ntuple_network.h"

// Greedy 1-ply agent over a learned n-tuple value function:
//
//     move = argmax over legal d of [ reward(d) + V(afterstate(d)) ]
//
// No search. All of the strength is in V, which is why the same class is used
// during training (see train_ntuple.cpp) and during play - the action rule must
// match, or the value function is evaluated under a policy it never saw.
class NTupleAI {
public:
    NTupleAI() = default;

    // True once weights have been loaded; an untrained network plays at random,
    // so callers should check this and say so rather than pretending.
    bool is_ready() const { return _ready; }
    bool load(const std::string& path);

    NTupleNetwork& network() { return _net; }
    const NTupleNetwork& network() const { return _net; }

    Direction get_move(const Game2048& game);

    // Best action by the greedy rule. Returns false when no move is legal
    // (terminal). `out_value` is reward + V(afterstate), which is exactly the
    // TD target for the previous afterstate.
    bool best_action(const Game2048& game, Direction& out_dir,
                     uint64_t& out_afterstate, int& out_reward,
                     double& out_value) const;

    double last_move_time = 0.0;   // milliseconds

private:
    NTupleNetwork _net;
    bool _ready = false;
};
