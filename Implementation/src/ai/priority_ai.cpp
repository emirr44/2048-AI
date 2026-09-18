#include "priority_ai.h"
#include "heuristics.h"
#include <cfloat>

// preferred move order: LEFT -> DOWN -> RIGHT -> UP
static const Direction PRIORITY_ORDER[] = {
    Direction::LEFT, Direction::DOWN, Direction::RIGHT, Direction::UP
};
static constexpr int PRIORITY_COUNT = 4;

Direction PriorityAI::get_move(const Game2048& game) {
    auto valid = game.get_valid_moves();
    if (valid.empty()) return Direction::LEFT;

    double best_score = -DBL_MAX;
    Direction best_move = valid[0];

    for (auto move : valid) {
        Game2048 copy = game.clone();
        copy.make_move(move);
        double score = evaluate_state(copy, move);
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }
    return best_move;
}
double PriorityAI::evaluate_state(const Game2048& game, Direction move) {
    int decoded[4][4];
    game.decode(decoded);
    double score = Heuristics::evaluate(decoded);

    // bottom-left corner gets high bonus for keeping highest tile there
    if (decoded[3][0] == game.highest_tile) {
        score += 80.0;
    }
    // directional preference as tiebreaker, based on move priority
    for (int i = 0; i < PRIORITY_COUNT; i++) {
        if (PRIORITY_ORDER[i] == move) {
            score += (PRIORITY_COUNT - i) * 0.1;
            break;
        }
    }
    return score;
}