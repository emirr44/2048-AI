#pragma once
#include "../core/game2048.h"
#include "../utils/helpers.h"

// greedy, 1-ply AI that tries each move, scores the result and picks the best
class PriorityAI {
public:
    Direction get_move(const Game2048& game);
private:
    double evaluate_state(const Game2048& game, Direction move);
};