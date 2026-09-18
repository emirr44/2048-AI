# 2048 — game, search agents and a reinforcement-learning agent

A 2048 implementation in C++ with a native GUI (natID) and five playing
agents, from a random baseline to a temporal-difference learner trained by
self-play.


## Build and run

Requires the natID SDK in `$HOME/natID.SDK` and CMake. On Windows build with
MSVC (natID ships MSVC import libraries); on macOS with Xcode.

## The agents

Several agents are implemented:

| Agent | Method |
|---|---|
| Random | uniformly random legal move; baseline |
| Priority | fixed move preference order; baseline |
| Expectimax | full expectimax search over the chance nodes, hand-written evaluation |
| TD N-Tuple | learned value function, one move deep |
| TD + Expectimax | the same learned values used as the evaluation inside the search |

**Expectimax** searches to an adaptive depth (deeper when the board is nearly
full, where every move matters), averages over every possible tile spawn with
its probability (90% a 2, 10% a 4), prunes branches whose cumulative
probability falls below 1e-4, and caches positions in a transposition table.
Its evaluation is four terms: empty cells (weight 270), available merges
(700), monotonicity as a cost with `rank⁴` (47), and a clutter penalty with
`rank^3.5` (11), plus a large penalty for a dead position. The fourth power is
what matters: it makes disorder next to a large tile far more expensive than
next to a small one.

**TD N-Tuple** is the reinforcement-learning agent. The value of a position is
a sum of table lookups,

    V(s) = Σ over patterns p, Σ over the 8 board symmetries t   W_p[ index(p, t(s)) ]

where a pattern is a set of board cells and its index is those cells' 4-bit
ranks packed together. Eight patterns of five cells give 8 × 16⁵ = 8.4 M
weights (32 MB as `float`). All eight symmetries of a pattern share one table,
which multiplies the training signal per board by eight and builds the game's
symmetry into the model instead of making it learn it.

The agent then plays greedily one move deep, choosing

    argmax over legal moves d of   reward(d) + V(afterstate(d))

**TD + Expectimax** keeps that value function but evaluates the leaves of the
expectimax search with it rather than with the hand-written terms, so search
and a learned evaluation compound. This is how the strongest published 2048
programs are built.

## Training the learning agent

Weights are learned by self-play with temporal-difference learning, TD(0), on
*afterstates* — the board after sliding but before the random tile appears:

    V(s'_t)  ←  V(s'_t) + α · [ r_(t+1) + V(s'_(t+1)) − V(s'_t) ]

Learning on afterstates removes the spawn randomness from the target, which is
why this converges far faster than learning V(state). Action selection stays
greedy throughout: the random spawns supply enough exploration.


### Results

After 200,000 self-play games:

| games | mean score | 2048 | 4096 | 8192 |
|---|---|---|---|---|
| 20,000 | 27,446 | 50% | 10% | — |
| 60,000 | 58,733 | 92% | 63% | 3% |
| 100,000 | 67,354 | 95% | 77% | 7% |
| 160,000 | 86,084 | 96% | 83% | 33% |
| 200,000 | 93,131 | 96% | 85% | 43% |

Measured afterwards on 300 games with learning switched off:

- **mean score 91,304**, best single game **173,236**
- **2048 in 95%**, **4096 in 83%**, **8192 in 44%** of games
- 16384 reached during training, in fewer than 1 game in 200

Szubert & Jaśkowski (2014), who introduced this method for
2048, report mean scores around 100,000.

## Notes

- **Human or AI** is chosen in the Mode dropdown; switching resets the board so
  a score is never carried over between players. The AI does not start by
  itself — press Start. Stop pauses it, Continue resumes the same board.
- **Evil Tiles** makes the spawned tile the worst possible one instead of a
  random one, which is a quick way to see how robust an agent is.
- **AI Step** advances a single move, useful for following a decision.
- The search statistics shown (depth, nodes, time) apply to the expectimax
  agents; the one-move TD agent reports only its time.
