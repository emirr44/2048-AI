#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/Button.h>
#include <gui/ComboBox.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include <gui/HorizontalLayout.h>
#include <gui/Timer.h>
#include "core/game2048.h"
#include "ai/random_ai.h"
#include "ai/priority_ai.h"
#include "ai/expectimax_ai.h"
#include "ai/ntuple_ai.h"

#ifndef GAME2048_PROJECT_ROOT
#define GAME2048_PROJECT_ROOT "."
#endif

class BoardCanvas;

class ControlPanel : public gui::View
{
private:
    // labels
    gui::Label _lbl_score;
    gui::Label _val_score;
    gui::Label _lbl_highest;
    gui::Label _val_highest;
    gui::Label _lbl_moves;
    gui::Label _val_moves;

    // best scores (human vs AI)
    gui::Label _lbl_best_manual;
    gui::Label _val_best_manual;
    gui::Label _lbl_best_ai;
    gui::Label _val_best_ai;
    int _best_manual_score = 0;
    int _best_ai_score     = 0;
    bool _is_human_mode    = true;

    // AI selection
    gui::Label    _lbl_ai;
    gui::ComboBox _cb_ai;
    gui::Label    _fill_ai;
    gui::Label    _fill_ai2;
    gui::Label    _fill_ai_step;

    // learned-weights status
    gui::Label _lbl_td;
    gui::Label _val_td;

    // expectimax stats
    gui::Label _lbl_depth;
    gui::Label _val_depth;
    gui::Label _lbl_nodes;
    gui::Label _val_nodes;
    gui::Label _lbl_time;
    gui::Label _val_time;

    // buttons
    gui::Button _btn_ai_step;
    gui::Button _btn_evil;
    gui::Label  _fill_evil;
    bool        _evil_mode = false;

    // layout
    gui::GridLayout _gl;

    // timer
    gui::Timer _timer;

    // ai agents
    RandomAI _random_ai;
    PriorityAI _priority_ai;
    ExpectimaxAI _expectimax_ai;
    NTupleAI _ntuple_ai;

    // combo box order
    enum AIKind { AI_Random = 0, AI_Priority, AI_Expectimax, AI_NTuple, AI_NTupleSearch };

    // references
    Game2048* _p_game   = nullptr;
    BoardCanvas* _p_canvas = nullptr;

    void update_labels() {
        // refreshes every label from the current game state
        if (!_p_game) {
            return;  // guards against a null game pointer
        }

        td::String str;
        str.format("%d", _p_game->score);
        _val_score.setTitle(str);

        str.format("%d", _p_game->highest_tile);
        _val_highest.setTitle(str);

        str.format("%d", _p_game->move_count);
        _val_moves.setTitle(str);

        str.format("%d", _best_manual_score);
        _val_best_manual.setTitle(str);

        str.format("%d", _best_ai_score);
        _val_best_ai.setTitle(str);

        if (_on_status_update)
            _on_status_update();
    }

    void update_expectimax_stats() {
        // pulls stats from the last AI move calculation
        td::String str;
        str.format("%d", _expectimax_ai.last_search_depth);
        _val_depth.setTitle(str);

        str.format("%d", _expectimax_ai.last_nodes_evaluated);
        _val_nodes.setTitle(str);

        str.format("%.1f ms", _expectimax_ai.last_search_time);
        _val_time.setTitle(str);
    }

    void do_ai_step();

public:
    ControlPanel()
    : _lbl_score("Score:")
    , _val_score("0")
    , _lbl_highest("Best Tile:")
    , _val_highest("0")
    , _lbl_moves("Moves:")
    , _val_moves("0")
    , _lbl_best_manual("Best (Manual):")
    , _val_best_manual("0")
    , _lbl_best_ai("Best (AI):")
    , _val_best_ai("0")
    , _lbl_ai("AI Algorithm:")
    , _fill_ai("")
    , _fill_ai2("")
    , _fill_ai_step("")
    , _lbl_td("TD weights:")
    , _val_td("-")
    , _lbl_depth("Search Depth:")
    , _val_depth("-")
    , _lbl_nodes("Nodes:")
    , _val_nodes("-")
    , _lbl_time("Time:")
    , _val_time("-")
    , _btn_ai_step("AI Step")
    , _btn_evil("Evil Tiles: OFF")
    , _fill_evil("")
    , _gl(13, 2)
    , _timer(this, 0.2f, false)
    , _expectimax_ai(10)
    {
        _cb_ai.addItem("Random");  // combo box set up
        _cb_ai.addItem("Priority");
        _cb_ai.addItem("Expectimax");
        _cb_ai.addItem("TD N-Tuple");
        _cb_ai.addItem("TD + Expectimax");
        _cb_ai.selectIndex(2);
        _cb_ai.setSizeLimits(130, Control::Limit::UseAsMin);

        // learned weights are optional: the two TD entries only work once
        // train_ntuple has produced a weights file
        if (_ntuple_ai.load(std::string(GAME2048_PROJECT_ROOT) + "/weights/ntuple.bin")) {
            td::String s;
            s.format("%lld games", _ntuple_ai.network().games_trained);
            _val_td.setTitle(s);
        }
        else {
            _val_td.setTitle("not trained");
        }

        _lbl_best_manual.setSizeLimitForNChars(15, Control::Limit::UseAsMin);
        _val_best_manual.setSizeLimitForNChars(7,  Control::Limit::UseAsMin);

        gui::GridComposer gc(_gl);  // grid layout
        gc.appendRow(_lbl_score)       << _val_score;
        gc.appendRow(_lbl_highest)     << _val_highest;
        gc.appendRow(_lbl_moves)       << _val_moves;
        gc.appendRow(_lbl_best_manual) << _val_best_manual;
        gc.appendRow(_lbl_best_ai)     << _val_best_ai;
        gc.appendRow(_lbl_ai)          << _fill_ai;
        gc.appendRow(_cb_ai)           << _fill_ai2;
        gc.appendRow(_btn_ai_step)     << _fill_ai_step;
        gc.appendRow(_btn_evil)        << _fill_evil;
        gc.appendRow(_lbl_td)          << _val_td;
        gc.appendRow(_lbl_depth)       << _val_depth;
        gc.appendRow(_lbl_nodes)       << _val_nodes;
        gc.appendRow(_lbl_time)        << _val_time;

        _gl.setMargins(16, 8);
        _gl.setSpaceBetweenCells(6, 16);

        setLayout(&_gl);

        _btn_evil.setType(gui::Button::Type::Destructive);

        _btn_ai_step.onClick([this]() { do_ai_step(); });  // step button
        _timer.onTimer([this]()       { do_ai_step(); });  // timer

        _btn_evil.onClick([this]()  // evil tiles button
        {
            _evil_mode = !_evil_mode;
            _btn_evil.setTitle(_evil_mode ? "Evil Tiles: ON" : "Evil Tiles: OFF");
            if (_p_canvas)
                _p_canvas->set_evil_mode(_evil_mode);
        });
    }

    std::function<void()> _on_status_update;

    void set_game(Game2048* p_game) {
        _p_game = p_game;
        update_labels();
    }
    void set_canvas(BoardCanvas* p_canvas) {
        _p_canvas = p_canvas;
    }
    void set_human_mode(bool v) {
        if (v == _is_human_mode) {
            return;
        }
        if (_p_game && _p_game->score > 0) {
            if (_is_human_mode) {
                if (_p_game->score > _best_manual_score) {
                    _best_manual_score = _p_game->score;
                }
            }
            else {
                if (_p_game->score > _best_ai_score)
                    _best_ai_score = _p_game->score;
            }
        }
        _is_human_mode = v;
        reset_game(false);
    }

    void on_board_changed() {
        // called after every move
        update_labels();
        if (_is_human_mode && _p_game && _p_game->is_game_over())
        {
            if (_p_game->score > _best_manual_score)
                _best_manual_score = _p_game->score;
            update_labels();
        }
    }
    void start_ai() {
        if (_is_human_mode) {
            return;
        }
        if (!_timer.isRunning()) {
            _timer.start();
        }
    }
    void stop_ai() {
        if (_timer.isRunning())
            _timer.stop();
    }

    void reset_game(bool save_current_score = true) {
        if (_timer.isRunning()) {
            _timer.stop();
        }

        if (save_current_score && _p_game && _p_game->score > 0) {
            if (_is_human_mode) {
                if (_p_game->score > _best_manual_score) {
                    _best_manual_score = _p_game->score;
                }
            }
            else {
                if (_p_game->score > _best_ai_score) {
                    _best_ai_score = _p_game->score;
                }
            }
        }

        *_p_game = Game2048();
        update_labels();
        _val_depth.setTitle("-");
        _val_nodes.setTitle("-");
        _val_time.setTitle("-");

        // a new board never starts playing by itself; the AI waits for Start
        _p_canvas->refresh();
    }

    bool is_ai_running() const {
        return _timer.isRunning();
    }

    // Continue is only meaningful on a board that is part-way through a game
    bool can_continue() const {
        return _p_game && _p_game->move_count > 0 && !_p_game->is_game_over();
    }
};

inline void ControlPanel::do_ai_step() {
    // core AI loop, called every 0.2 seconds
    if (!_p_game || !_p_canvas) {
        return;
    }
    if (_is_human_mode) {
        return;
    }

    int ai_idx = _cb_ai.getSelectedIndex();

    if (_p_game->is_game_over()) {
        if (_timer.isRunning()) {
            _timer.stop();
        }
        if (_p_game->score > _best_ai_score) {
            _best_ai_score = _p_game->score;
        }
        update_labels();
        return;
    }

    // the two TD options need trained weights; refuse rather than play at random
    if ((ai_idx == AI_NTuple || ai_idx == AI_NTupleSearch) && !_ntuple_ai.is_ready()) {
        if (_timer.isRunning()) {
            _timer.stop();
        }
        _val_td.setTitle("run train_ntuple");
        return;
    }

    // only the TD + Expectimax option evaluates leaves with the learned values,
    // so plain Expectimax stays a clean baseline to compare against
    _expectimax_ai.set_value_network(
        ai_idx == AI_NTupleSearch ? &_ntuple_ai.network() : nullptr);

    Direction move;
    switch (ai_idx) {
        case AI_Random:   move = _random_ai.get_move(*_p_game);     break;
        case AI_Priority: move = _priority_ai.get_move(*_p_game);   break;
        case AI_NTuple:   move = _ntuple_ai.get_move(*_p_game);     break;
        default:          move = _expectimax_ai.get_move(*_p_game); break;
    }

    _p_game->make_move(move);
    if (_evil_mode) {
        _p_game->spawn_evil_tile();
    }
    else {
        _p_game->spawn_tile();
    }

    if (ai_idx == AI_Expectimax || ai_idx == AI_NTupleSearch) {
        update_expectimax_stats();
    }
    else if (ai_idx == AI_NTuple) {
        // 1-ply lookup: depth and node count are meaningless, only timing is
        _val_depth.setTitle("1");
        _val_nodes.setTitle("-");
        td::String str;
        str.format("%.2f ms", _ntuple_ai.last_move_time);
        _val_time.setTitle(str);
    }
    update_labels();
    _p_canvas->refresh();
}
