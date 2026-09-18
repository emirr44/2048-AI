#include "ntuple_ai.h"

#include <chrono>
#include <limits>

bool NTupleAI::load(const std::string& path) {
    _ready = _net.load(path);
    return _ready;
}

bool NTupleAI::best_action(const Game2048& game, Direction& out_dir,
                           uint64_t& out_afterstate, int& out_reward,
                           double& out_value) const {
    bool found = false;
    double best = -std::numeric_limits<double>::infinity();

    for (Direction d : ALL_DIRECTIONS) {
        auto [next, legal] = game.simulate_move(d);
        if (!legal) continue;

        // make_move() slides without spawning, so next.board is the afterstate
        // and the score delta is the immediate reward.
        const int reward = next.score - game.score;
        const double v = reward + _net.value(next.board);

        if (!found || v > best) {
            found = true;
            best = v;
            out_dir = d;
            out_afterstate = next.board;
            out_reward = reward;
        }
    }

    out_value = best;
    return found;
}

Direction NTupleAI::get_move(const Game2048& game) {
    const auto t0 = std::chrono::steady_clock::now();

    Direction dir = Direction::UP;
    uint64_t afterstate = 0;
    int reward = 0;
    double value = 0.0;

    if (!best_action(game, dir, afterstate, reward, value)) {
        // Terminal; any direction is as good as another. Caller checks
        // is_game_over() anyway.
        dir = Direction::UP;
    }

    last_move_time = std::chrono::duration<double>(
                         std::chrono::steady_clock::now() - t0).count() * 1e3;
    return dir;
}
