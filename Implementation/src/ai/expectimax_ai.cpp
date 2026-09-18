#include "expectimax_ai.h"
#include "heuristics.h"
#include "ntuple_network.h"
#include <cfloat>
#include <algorithm>

ExpectimaxAI::ExpectimaxAI(int max_depth) : max_depth(max_depth), nodes_evaluated(0) {
    cache.reserve(1 << 18);  // pre-allocate ~260k buckets to avoid rehashing (estimate)
}

Direction ExpectimaxAI::get_move(const Game2048& game) {
    auto start_time = std::chrono::high_resolution_clock::now();

    auto valid = game.get_valid_moves();
    if (valid.empty()) return Direction::LEFT;
    int depth = adaptive_depth(game);
    last_search_depth = depth;

    // iterative deepening: instead of searching directly at depth, search at
    // depth 1, then 2, 3... this benefits move ordering (most promising
    // branch is explored first)

    Direction best_move = valid[0];
    cache.clear();  // clear once before all iterations so entries carry over between depths
    for (int d = 1; d <= depth; d++) {
        nodes_evaluated = 0;
        double iterative_best_score = -DBL_MAX;
        Direction iterative_best_move = valid[0];

        // try the previous iteration's best move first (move ordering); this
        // improves cache hit rates in deeper searches
        std::vector<Direction> ordered = valid;
        auto it = std::find(ordered.begin(), ordered.end(), best_move);  // move best to front
        if (it != ordered.begin() && it != ordered.end()) {
            std::rotate(ordered.begin(), it, it + 1);
        }
        for (auto move : ordered) {
            Game2048 copy = game.clone();
            copy.make_move(move);  // clone board and apply move
            double score = expectimax(copy, d - 1, false, 1.0);
            if (score > iterative_best_score) {
                iterative_best_score = score;
                iterative_best_move = move;
            }
        }
        best_move = iterative_best_move;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    last_search_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    last_nodes_evaluated = nodes_evaluated;

    return best_move;
}
int ExpectimaxAI::adaptive_depth(const Game2048& game) const {
    // fewer empty cells = more critical position = search deeper: with many
    // empty cells the game is safe and shallow search suffices, with few
    // empty cells every move matters
    int empty = game.count_empty_cells();

    if (empty <= 2)  return std::min(max_depth, 10);  // tiny tree, deep search is cheap here
    if (empty <= 4)  return std::min(max_depth, 8);
    if (empty <= 6)  return std::min(max_depth, 7);
    if (empty <= 8)  return std::min(max_depth, 7);
    return std::min(max_depth, 6);   // plenty of room
}

// leaf evaluation: the learned value function when one is attached, otherwise
// the hand-written heuristic
static double eval(const Game2048& game, const NTupleNetwork* net) {
    if (net) {
        return net->value(game.board);
    }
    // the bitboard already stores ranks, so read them straight out instead of
    // decoding to values and taking log2 back again
    int ranks[4][4];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            ranks[r][c] = get_tile(game.board, r * 4 + c);
    return Heuristics::evaluate_ranks(ranks);
}

// a terminal position must be worse than any position that is still alive
static double eval_terminal(const Game2048& game, const NTupleNetwork* net) {
    return eval(game, net) - Heuristics::LOST_PENALTY;
}

double ExpectimaxAI::expectimax(Game2048& game, int depth, bool is_max, double probability) {
    nodes_evaluated++;

    if (probability < PROBABILITY_THRESHOLD) {
        return eval(game, _value_net);  // prune astronomically unlikely branches
    }
    if (game.is_game_over()) {
        return eval_terminal(game, _value_net);
    }
    if (depth == 0) {
        return eval(game, _value_net);
    }
    uint64_t board_hash = game.hash();
    uint64_t c_key = cache_key(board_hash, depth, is_max);
    auto it = cache.find(c_key);
    // the key mixes the 64-bit board and so can collide; without comparing
    // the board, a hit can return another position's value
    if (it != cache.end() && it->second.board == board_hash && it->second.depth >= depth) {
        return it->second.value;
    }
    double result;
    if (is_max) {
        result = max_node(game, depth, probability);
    } else {
        result = chance_node(game, depth, probability);
    }
    cache[c_key] = {result, depth, board_hash};
    return result;
}
double ExpectimaxAI::max_node(Game2048& game, int depth, double probability) {
    auto valid = game.get_valid_moves();
    if (valid.empty())
        return eval_terminal(game, _value_net);   // no move available: lost

    double max_value = -DBL_MAX;
    for (auto move : valid) {
        Game2048 copy = game.clone();
        copy.make_move(move);
        double val = expectimax(copy, depth - 1, false, probability);
        max_value = std::max(max_value, val);
    }
    return max_value;
}
double ExpectimaxAI::chance_node(Game2048& game, int depth, double probability) {
    auto empty = game.get_empty_cells();
    if (empty.empty()) {
        return eval(game, _value_net);
    }
    int num_empty = static_cast<int>(empty.size());
    double cell_p = 1.0 / num_empty;  // each empty cell has equal chance of being chosen
    double total = 0.0;

    for (auto& [r, c] : empty) {
        int idx = r * 4 + c;
        // spawn a 2 (rank 1) with 90% chance
        {
            double branch_prob = probability * cell_p * 0.9;
            Game2048 copy = game.clone();
            copy.board = set_tile(copy.board, idx, 1);
            double val = expectimax(copy, depth - 1, true, branch_prob);
            total += 0.9 * cell_p * val;
        }
        // spawn a 4 (rank 2) with 10% chance
        {
            double branch_prob = probability * cell_p * 0.1;
            Game2048 copy = game.clone();
            copy.board = set_tile(copy.board, idx, 2);
            double val = expectimax(copy, depth - 1, true, branch_prob);
            total += 0.1 * cell_p * val;
        }
    }
    return total;
}

uint64_t ExpectimaxAI::cache_key(uint64_t board_hash, int depth, bool is_max) {
    // polynomial mixing: avoids XOR collisions with board bits at positions 55-63
    return board_hash * 1000003ULL
         + static_cast<uint64_t>(depth) * 1000033ULL
         + static_cast<uint64_t>(is_max);
}