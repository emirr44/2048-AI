#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../core/game2048.h"

// N-tuple network value function for 2048, trained by temporal-difference
// learning (Szubert & Jaskowski 2014, "Temporal difference learning of N-tuple
// networks for the game 2048").
//
// V(board) is a plain sum of table lookups:
//
//     V(s) = sum over patterns p, sum over the 8 board symmetries t
//              W_p[ index(p, t(s)) ]
//
// A "pattern" is a set of board cells.  Its table has 16^|p| entries, indexed
// by packing the 4-bit tile ranks of those cells together - which is why the
// uint64_t bitboard in Game2048 suits this so well: an index is a handful of
// shifts, with no board decoding.
//
// All 8 symmetries of a pattern share a single weight table (symmetric
// sampling).  That multiplies the training signal per board by 8 and bakes the
// game's symmetry into the model instead of forcing it to be learned.
//
// This is linear function approximation over binary features, so it is fast to
// evaluate, trains on a CPU, and cannot diverge the way a neural network can.
class NTupleNetwork {
public:
    static constexpr int kSymmetries = 8;

    // Builds the default pattern set: 8 patterns of 5 cells, 16^5 entries each,
    // ~34 MB of float weights in total.
    //
    // To trade memory for strength, extend the base patterns in the .cpp to 6
    // cells: that is the configuration the strongest published results use, at
    // 16^6 = 16.8M entries (67 MB) per pattern.  Nothing else has to change -
    // table sizes follow the pattern lengths.
    NTupleNetwork();

    double value(uint64_t board) const;

    // Distributes `delta` over every weight this board touches, so that
    // value(board) moves by approximately `delta`.
    void update(uint64_t board, double delta);

    bool save(const std::string& path) const;
    // Fails if the file is missing, malformed, or built from a different
    // pattern set (stale weights are worse than none).
    bool load(const std::string& path);

    std::size_t pattern_count() const { return _patterns.size(); }
    std::size_t total_weights() const;
    double memory_mb() const;

    long long games_trained = 0;

private:
    struct Pattern {
        std::vector<int> base_cells;                       // as declared
        int cells[kSymmetries][8] = {};                    // the 8 symmetric variants
        int length = 0;
        std::vector<float> weights;
    };

    std::vector<Pattern> _patterns;

    // Number of weights touched per board, i.e. patterns * 8.
    int _touched_per_board = 0;

    static void build_symmetries(Pattern& p);
};
