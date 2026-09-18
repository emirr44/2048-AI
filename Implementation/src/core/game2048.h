#pragma once
#include <vector>
#include <utility>
#include <cstdint>
#include <functional>
#include "../utils/helpers.h"

// Board is a uint64_t: each cell occupies 4 bits at position (r*4+c)*4
// rank 0 = empty, rank n = tile value 2^n (so 2→1, 4→2, 8→3, ...)
inline int  get_tile(uint64_t board, int idx)            { return (board >> (idx * 4)) & 0xF; }
inline uint64_t set_tile(uint64_t board, int idx, int rank) {
    int shift = idx * 4;
    return (board & ~(uint64_t(0xF) << shift)) | (uint64_t(rank & 0xF) << shift);
}

class Game2048 {
public:
    uint64_t board;
    int score;
    int highest_tile;
    int move_count;

    explicit Game2048(bool spawn_tiles = true);

    std::vector<std::pair<int, int>> get_empty_cells() const;
    int count_empty_cells() const;
    int get_max_tile() const;

    bool spawn_tile();
    bool spawn_evil_tile();

    Game2048 clone() const;

    bool make_move(Direction dir);
    std::pair<Game2048, bool> simulate_move(Direction dir) const;

    bool can_move(Direction dir) const;
    std::vector<Direction> get_valid_moves() const;
    bool is_game_over() const;
    bool has_won() const;

    void print_board() const;

    uint64_t hash() const { return board; }

    // Decode bitboard to int[4][4] for heuristics
    void decode(int out[4][4]) const;

    // Row move table: indexed by 16-bit row (4 nibbles), gives result row + score
    struct RowResult { uint16_t row; int score; };
    static RowResult row_table[65536];
    static bool      row_table_ready;

    static uint16_t  extract_row(uint64_t board, int r);
    static uint64_t  place_row(uint64_t board, int r, uint16_t row);

private:
    void update_highest();

    static void      build_row_table();
    static uint64_t  rotate_cw(uint64_t board);
    static uint64_t  rotate_ccw(uint64_t board);
    static uint64_t  mirror_rows(uint64_t board);
};
