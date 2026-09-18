#pragma once

// Board evaluation for the expectimax search.
//
// Two entry points:
//   evaluate()       - board holds tile VALUES (2, 4, 8, ...). Convenience
//                      wrapper, used by Game2048::spawn_evil_tile().
//   evaluate_ranks() - board holds tile RANKS (0 = empty, n = tile 2^n). This
//                      is the search's hot path: the bitboard already stores
//                      ranks, so nothing has to be converted and no
//                      transcendental function is called per leaf.
class Heuristics {
public:
    static double evaluate(const int board[4][4]);
    static double evaluate_ranks(const int ranks[4][4]);

    // Subtracted by the search when a position is terminal, so that dying is
    // always worse than any surviving line.
    static constexpr double LOST_PENALTY = 200000.0;

    // Individual terms, over ranks, exposed for reporting/tuning.
    static double empty_cells(const int ranks[4][4]);
    static double merges(const int ranks[4][4]);
    static double monotonicity(const int ranks[4][4]);
    static double tile_sum(const int ranks[4][4]);
};
