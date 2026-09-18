// train_ntuple - self-play TD(0) training of the n-tuple value function.
//
// Console tool, CPU only. Nothing here touches the GUI, so it can run for hours
// in a terminal while you keep working.
//
//   train_ntuple --games 200000 --alpha 0.1
//   train_ntuple --resume weights/ntuple.bin --games 200000   (continue)
//   train_ntuple --resume weights/ntuple.bin --games 0 --eval 500   (measure only)
//
// The learning rule is afterstate TD(0):
//
//     V(s'_t) <- V(s'_t) + alpha * [ r_{t+1} + V(s'_{t+1}) - V(s'_t) ]
//
// where s'_t is the board after sliding but before the random tile appears.
// Learning on afterstates rather than states removes the spawn randomness from
// the target, which is what makes this converge so much faster than learning
// V(state) directly.
//
// Action selection is greedy throughout; no epsilon-greedy is needed because
// the random tile spawns supply the exploration.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../ai/ntuple_ai.h"
#include "../ai/ntuple_network.h"
#include "../core/game2048.h"
#include "../utils/helpers.h"

#ifndef GAME2048_PROJECT_ROOT
#define GAME2048_PROJECT_ROOT "."
#endif

namespace {

struct Options {
    long long games = 100000;
    double alpha = 0.1;
    long long report = 1000;
    long long eval_games = 200;
    unsigned seed = 0;             // 0 => nondeterministic
    std::string out = std::string(GAME2048_PROJECT_ROOT) + "/weights/ntuple.bin";
    std::string resume;
};

// Rolling statistics over a block of games.
struct Block {
    long long games = 0;
    double score_sum = 0.0;
    int best_score = 0;
    std::map<int, long long> max_tile_counts;

    void add(int score, int max_tile) {
        ++games;
        score_sum += score;
        best_score = std::max(best_score, score);
        ++max_tile_counts[max_tile];
    }

    double mean() const { return games ? score_sum / double(games) : 0.0; }

    // Fraction of games whose largest tile reached at least `tile`.
    double rate_at_least(int tile) const {
        long long n = 0;
        for (const auto& [t, c] : max_tile_counts) {
            if (t >= tile) n += c;
        }
        return games ? double(n) / double(games) : 0.0;
    }

    std::string tile_summary() const {
        std::string s;
        char buf[64];
        for (int tile : {512, 1024, 2048, 4096, 8192, 16384, 32768}) {
            const double r = rate_at_least(tile);
            if (r <= 0.0) continue;
            std::snprintf(buf, sizeof(buf), "%d:%.0f%% ", tile, r * 100.0);
            s += buf;
        }
        return s;
    }
};

// One self-play game with learning. Returns the final score.
int train_one_game(NTupleAI& agent, double alpha, int& out_max_tile) {
    NTupleNetwork& net = agent.network();
    Game2048 game;   // constructor spawns the two starting tiles

    bool have_prev = false;
    uint64_t prev_afterstate = 0;

    for (;;) {
        Direction dir;
        uint64_t afterstate = 0;
        int reward = 0;
        double value = 0.0;   // reward + V(afterstate) for the chosen move

        if (!agent.best_action(game, dir, afterstate, reward, value)) {
            break;            // no legal move: terminal
        }

        // `value` is the TD target for the afterstate we produced last step.
        if (have_prev) {
            const double v_prev = net.value(prev_afterstate);
            net.update(prev_afterstate, alpha * (value - v_prev));
        }

        game.make_move(dir);
        game.spawn_tile();

        prev_afterstate = afterstate;
        have_prev = true;
    }

    // The game ended, so the final afterstate has no successor: target is 0.
    if (have_prev) {
        const double v_prev = net.value(prev_afterstate);
        net.update(prev_afterstate, alpha * (0.0 - v_prev));
    }

    out_max_tile = game.highest_tile;
    return game.score;
}

// Greedy play with learning switched off, to measure the current policy.
int play_one_game(NTupleAI& agent, int& out_max_tile) {
    Game2048 game;
    for (;;) {
        Direction dir;
        uint64_t afterstate = 0;
        int reward = 0;
        double value = 0.0;
        if (!agent.best_action(game, dir, afterstate, reward, value)) break;
        game.make_move(dir);
        game.spawn_tile();
    }
    out_max_tile = game.highest_tile;
    return game.score;
}

bool parse_args(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const bool has_next = (i + 1 < argc);
        auto next = [&]() { return std::string(argv[++i]); };

        if (a == "--games" && has_next)        o.games = std::atoll(next().c_str());
        else if (a == "--alpha" && has_next)   o.alpha = std::atof(next().c_str());
        else if (a == "--report" && has_next)  o.report = std::atoll(next().c_str());
        else if (a == "--eval" && has_next)    o.eval_games = std::atoll(next().c_str());
        else if (a == "--seed" && has_next)    o.seed = unsigned(std::atoll(next().c_str()));
        else if (a == "--out" && has_next)     o.out = next();
        else if (a == "--resume" && has_next)  o.resume = next();
        else if (a == "--help" || a == "-h")   return false;
        else {
            std::printf("unknown or incomplete option: %s\n", a.c_str());
            return false;
        }
    }
    if (o.report <= 0) o.report = 1000;
    return true;
}

void usage() {
    std::printf(
        "train_ntuple - TD(0) training of the 2048 n-tuple value function\n\n"
        "  --games N     self-play training games (default 100000)\n"
        "  --alpha A     TD learning rate (default 0.1)\n"
        "  --report K    print stats and checkpoint every K games (default 1000)\n"
        "  --eval N      greedy games to measure after training (default 200)\n"
        "  --seed S      RNG seed for reproducibility (default: random)\n"
        "  --out PATH    where to write weights\n"
        "  --resume PATH load weights before training, to continue a run\n");
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parse_args(argc, argv, opt)) {
        usage();
        return 2;
    }

    if (opt.seed != 0) seed_rng(opt.seed);

    NTupleAI agent;
    std::printf("n-tuple network: %zu patterns, %zu weights, %.1f MB\n",
                agent.network().pattern_count(), agent.network().total_weights(),
                agent.network().memory_mb());

    if (!opt.resume.empty()) {
        if (!agent.load(opt.resume)) {
            std::printf("could not load weights from %s\n", opt.resume.c_str());
            std::printf("(missing file, or built from a different pattern set)\n");
            return 1;
        }
        std::printf("resumed from %s (%lld games already trained)\n",
                    opt.resume.c_str(), agent.network().games_trained);
    }

    std::printf("training %lld games, alpha=%.4f\n\n", opt.games, opt.alpha);
    std::printf("%10s %12s %12s  %s\n", "games", "mean score", "best", "max tile reached");
    std::fflush(stdout);

    const auto t_start = std::chrono::steady_clock::now();
    Block block;
    Block overall;

    for (long long g = 1; g <= opt.games; ++g) {
        int max_tile = 0;
        const int score = train_one_game(agent, opt.alpha, max_tile);
        block.add(score, max_tile);
        overall.add(score, max_tile);
        ++agent.network().games_trained;

        if (g % opt.report == 0 || g == opt.games) {
            std::printf("%10lld %12.0f %12d  %s\n", agent.network().games_trained,
                        block.mean(), block.best_score, block.tile_summary().c_str());
            std::fflush(stdout);
            block = Block{};

            // Checkpoint, so a long run is never lost to an interruption.
            if (!agent.network().save(opt.out)) {
                std::printf("WARNING: could not write %s\n", opt.out.c_str());
            }
        }
    }

    const double mins = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - t_start).count() / 60.0;

    if (opt.games > 0) {
        std::printf("\ntrained %lld games in %.1f min (mean %.0f over the whole run)\n",
                    opt.games, mins, overall.mean());
        if (!agent.network().save(opt.out)) {
            std::printf("could not write %s\n", opt.out.c_str());
            return 1;
        }
        std::printf("weights written to %s\n", opt.out.c_str());
    }

    if (opt.eval_games > 0) {
        std::printf("\nevaluating %lld greedy games (no learning)...\n", opt.eval_games);
        Block ev;
        for (long long i = 0; i < opt.eval_games; ++i) {
            int max_tile = 0;
            const int score = play_one_game(agent, max_tile);
            ev.add(score, max_tile);
        }
        std::printf("mean score %.0f, best %d\n", ev.mean(), ev.best_score);
        std::printf("reached: %s\n", ev.tile_summary().c_str());
    }

    return 0;
}
