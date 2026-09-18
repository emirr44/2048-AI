#include "heuristics.h"

#include <algorithm>
#include <cmath>

// Weights follow the structure of the well-known open-source expectimax
// implementation for 2048: reward space and merges, punish disorder and
// clutter. The exponents are the important part - see monotonicity() below.
static constexpr double W_EMPTY        = 270.0;
static constexpr double W_MERGES       = 700.0;
static constexpr double W_MONOTONICITY = 47.0;
static constexpr double W_SUM          = 11.0;

static constexpr double MONOTONICITY_POWER = 4.0;
static constexpr double SUM_POWER          = 3.5;

namespace {

// Ranks are 0..15, so every power we need fits in a 16-entry table computed
// once. This keeps pow() out of the search entirely; the old code called
// std::log2 roughly 170 times per leaf, which dominated the whole search.
struct PowTables {
    double mono[16];
    double sum[16];

    PowTables() {
        for (int r = 0; r < 16; ++r) {
            mono[r] = std::pow(double(r), MONOTONICITY_POWER);
            sum[r]  = std::pow(double(r), SUM_POWER);
        }
    }
};

const PowTables& tables() {
    static const PowTables t;
    return t;
}

// Per-line contribution. Called for each of the 4 rows and 4 columns, which is
// how the terms end up covering both axes.
struct LineScore {
    int empty = 0;
    int merges = 0;
    double sum = 0.0;
    double monotonicity = 0.0;   // cost: 0 for a perfectly ordered line
};

LineScore score_line(const int line[4]) {
    const PowTables& t = tables();
    LineScore s;

    // empty cells, clutter, and runs of equal tiles
    int counter = 0;
    int prev = 0;
    for (int i = 0; i < 4; ++i) {
        const int rank = line[i];
        s.sum += t.sum[rank];
        if (rank == 0) {
            ++s.empty;
            continue;
        }
        if (prev == rank) {
            ++counter;
        }
        else if (counter > 0) {
            s.merges += 1 + counter;
            counter = 0;
        }
        prev = rank;
    }
    if (counter > 0) s.merges += 1 + counter;

    // Monotonicity as a cost, measured in BOTH directions; the cheaper
    // direction wins, so a line ordered either way is free.
    //
    // The 4th power is what makes this work: breaking order next to a 16384
    // costs vastly more than breaking it next to a 4. A linear (or log-scaled)
    // version treats those roughly alike, which is why the previous evaluation
    // mismanaged exactly the large tiles that decide the endgame.
    double left = 0.0, right = 0.0;
    for (int i = 1; i < 4; ++i) {
        const double a = t.mono[line[i - 1]];
        const double b = t.mono[line[i]];
        if (a > b) left  += a - b;
        else       right += b - a;
    }
    s.monotonicity = std::min(left, right);

    return s;
}

}  // namespace

double Heuristics::evaluate_ranks(const int ranks[4][4]) {
    double empty = 0.0, merge_total = 0.0, sum = 0.0, mono = 0.0;

    for (int r = 0; r < 4; ++r) {
        const int row[4] = {ranks[r][0], ranks[r][1], ranks[r][2], ranks[r][3]};
        const LineScore s = score_line(row);
        empty += s.empty;
        merge_total += s.merges;
        sum += s.sum;
        mono += s.monotonicity;
    }
    for (int c = 0; c < 4; ++c) {
        const int col[4] = {ranks[0][c], ranks[1][c], ranks[2][c], ranks[3][c]};
        const LineScore s = score_line(col);
        // empty and sum would be double counted across rows and columns, so
        // only the directional terms are taken from the column pass.
        merge_total += s.merges;
        mono += s.monotonicity;
    }

    return W_EMPTY * empty
         + W_MERGES * merge_total
         - W_MONOTONICITY * mono
         - W_SUM * sum;
}

double Heuristics::evaluate(const int board[4][4]) {
    // values -> ranks; only used off the hot path
    int ranks[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            int rank = 0;
            int v = board[r][c];
            while (v > 1) { v >>= 1; ++rank; }
            ranks[r][c] = rank;
        }
    }
    return evaluate_ranks(ranks);
}

double Heuristics::empty_cells(const int ranks[4][4]) {
    int count = 0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (ranks[r][c] == 0) ++count;
    return double(count);
}

double Heuristics::merges(const int ranks[4][4]) {
    double total = 0.0;
    for (int r = 0; r < 4; ++r) {
        const int row[4] = {ranks[r][0], ranks[r][1], ranks[r][2], ranks[r][3]};
        total += score_line(row).merges;
    }
    for (int c = 0; c < 4; ++c) {
        const int col[4] = {ranks[0][c], ranks[1][c], ranks[2][c], ranks[3][c]};
        total += score_line(col).merges;
    }
    return total;
}

double Heuristics::monotonicity(const int ranks[4][4]) {
    double total = 0.0;
    for (int r = 0; r < 4; ++r) {
        const int row[4] = {ranks[r][0], ranks[r][1], ranks[r][2], ranks[r][3]};
        total += score_line(row).monotonicity;
    }
    for (int c = 0; c < 4; ++c) {
        const int col[4] = {ranks[0][c], ranks[1][c], ranks[2][c], ranks[3][c]};
        total += score_line(col).monotonicity;
    }
    return total;
}

double Heuristics::tile_sum(const int ranks[4][4]) {
    double total = 0.0;
    for (int r = 0; r < 4; ++r) {
        const int row[4] = {ranks[r][0], ranks[r][1], ranks[r][2], ranks[r][3]};
        total += score_line(row).sum;
    }
    return total;
}
