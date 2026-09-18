#pragma once
#include <gui/View.h>
#include <gui/Button.h>
#include <gui/ComboBox.h>
#include <gui/Label.h>
#include <gui/HorizontalLayout.h>
#include <gui/SplitterLayout.h>
#include "BoardCanvas.h"
#include "ControlPanel.h"

// top bar: mode is a dropdown, playback is Stop / Continue
class ButtonsBar : public gui::View {
private:
    gui::Label _lbl_mode;
    gui::ComboBox _cb_mode;
    gui::Button _btn_start;
    gui::Button _btn_stop;
    gui::Button _btn_continue;
    gui::Button _btn_new_game;
    gui::HorizontalLayout _hl;

public:
    std::function<void(bool)> _on_mode_changed;
    std::function<void()> _on_start;
    std::function<void()> _on_stop;
    std::function<void()> _on_continue;
    std::function<void()> _on_new_game;

    ButtonsBar()
    : _lbl_mode("Mode:")
    , _btn_start("Start")
    , _btn_stop("Stop")
    , _btn_continue("Continue")
    , _btn_new_game("New Game")
    , _hl(8)
    {
        _cb_mode.addItem("Human");
        _cb_mode.addItem("AI");
        _cb_mode.selectIndex(0, false);   // start in human mode, without firing the handler
        _cb_mode.setSizeLimits(110, Control::Limit::UseAsMin);

        _btn_start.setType(gui::Button::Type::Constructive);
        _btn_stop.setType(gui::Button::Type::Destructive);
        _btn_continue.setType(gui::Button::Type::Constructive);

        _hl.append(_lbl_mode);
        _hl.append(_cb_mode);
        _hl.appendSpace(16);
        _hl.append(_btn_start);
        _hl.append(_btn_stop);
        _hl.append(_btn_continue);
        _hl.append(_btn_new_game);
        _hl.appendSpacer();
        setLayout(&_hl);

        _cb_mode.onChangedSelection([this]() {
            if (_on_mode_changed) _on_mode_changed(is_ai_mode());
        });

        _btn_start.onClick([this]() {
            if (_on_start) _on_start();
        });

        _btn_stop.onClick([this]() {
            if (_on_stop) _on_stop();
        });

        _btn_continue.onClick([this]() {
            if (_on_continue) _on_continue();
        });

        _btn_new_game.onClick([this]() {
            if (_on_new_game) _on_new_game();
        });

        update_buttons(false, false, false);
    }

    bool is_ai_mode() const {
        return _cb_mode.getSelectedIndex() == 1;
    }

    // Start begins a fresh run, Continue resumes a board that was stopped
    // part-way, Stop halts a running one. None of them apply to a human game.
    void update_buttons(bool ai_mode, bool running, bool can_continue) {
        _btn_start.enable(ai_mode && !running);
        _btn_stop.enable(ai_mode && running);
        _btn_continue.enable(ai_mode && !running && can_continue);
    }
};

// bottom bar
class BottomBar : public gui::View {
private:
    gui::Label _lbl_status;
    gui::Label _val_status;
    gui::HorizontalLayout _hl;
    Game2048* _p_game = nullptr;

public:
    BottomBar()
    : _lbl_status("Status:")
    , _val_status("Playing")
    , _hl(8)
    {
        _lbl_status.setFont(gui::Font::ID::SystemLarger);
        _lbl_status.setSizeLimits(0, Control::Limit::None, 50, Control::Limit::Fixed);
        _val_status.setFont(gui::Font::ID::SystemLarger);
        _val_status.setSizeLimits(0, Control::Limit::None, 50, Control::Limit::Fixed);
        _hl.appendSpacer();
        _hl.append(_lbl_status);
        _hl.append(_val_status);
        _hl.appendSpacer();
        setLayout(&_hl);
    }

    void set_game(Game2048* p_game) {
        _p_game = p_game;
    }

    void update() {
        if (!_p_game) return;
        if (_p_game->has_won()) {
            _val_status.setTitle("You Win!");
        }
        else if (_p_game->is_game_over()) {
            _val_status.setTitle("Game Over");
        }
        else
            _val_status.setTitle("Playing");
    }
};

// board area: canvas (fills) + BottomBar (fixed at bottom)
class BoardWithBars : public gui::View
{
private:
    BoardCanvas _canvas;
    BottomBar   _bottom_bar;
    gui::SplitterLayout _v_splitter;

public:
    std::function<void()> _on_board_changed;

    BoardWithBars()
    : _v_splitter(gui::SplitterLayout::Orientation::Vertical, gui::SplitterLayout::AuxiliaryCell::Second) {
        _v_splitter.setContent(_canvas, _bottom_bar);
        setLayout(&_v_splitter);

        _canvas._on_board_changed = [this]() {
            _bottom_bar.update();  // status is updated when canvas reports board change
            if (_on_board_changed) {
                _on_board_changed();
            }
        };
    }

    void set_game(Game2048* p_game) {
        _canvas.set_game(p_game);
        _bottom_bar.set_game(p_game);
    }

    BoardCanvas& get_canvas() { 
        return _canvas;
    }
    void update_status() {
        _bottom_bar.update();
    }
    void set_human_mode(bool v) {
        _canvas.set_human_mode(v);
    }
};

// canvas panel: ButtonsBar (top) + BoardWithBars (fills)
class CanvasPanel : public gui::View {
private:
    ButtonsBar _buttons_bar;
    BoardWithBars _board_with_bars;
    gui::SplitterLayout _v_splitter;

public:
    std::function<void()> _on_board_changed;

    CanvasPanel()
    : _v_splitter(gui::SplitterLayout::Orientation::Vertical, gui::SplitterLayout::AuxiliaryCell::First)
    {
        _v_splitter.setContent(_buttons_bar, _board_with_bars);
        setLayout(&_v_splitter);

        _board_with_bars._on_board_changed = [this]()
        {
            if (_on_board_changed) {
                _on_board_changed();
            }
        };
    }

    void set_game(Game2048* p_game) { 
        _board_with_bars.set_game(p_game);
    }
    BoardCanvas& get_canvas() {
        return _board_with_bars.get_canvas();
    }
    void update_status() {
        _board_with_bars.update_status();
    }
    void set_mode_callback(std::function<void(bool)> fn) {
        _buttons_bar._on_mode_changed = fn;
    }
    void set_start_callback(std::function<void()> fn) {
        _buttons_bar._on_start = fn;
    }
    void set_stop_callback(std::function<void()> fn) {
        _buttons_bar._on_stop = fn;
    }
    void set_continue_callback(std::function<void()> fn) {
        _buttons_bar._on_continue = fn;
    }
    void set_new_game_callback(std::function<void()> fn) {
        _buttons_bar._on_new_game = fn;
    }
    void set_human_mode(bool v) {
        _board_with_bars.set_human_mode(v);
    }
    bool is_ai_mode() const {
        return _buttons_bar.is_ai_mode();
    }
    void update_buttons(bool ai_mode, bool running, bool can_continue) {
        _buttons_bar.update_buttons(ai_mode, running, can_continue);
    }
};

// main view: CanvasPanel (fills) + ControlPanel (right, auxiliary)
class MainView : public gui::View {
private:
    Game2048 _game;
    CanvasPanel _canvas_panel;
    ControlPanel _control_panel;
    gui::SplitterLayout _splitter;

public:
    MainView()
    : _splitter(gui::SplitterLayout::Orientation::Horizontal, gui::SplitterLayout::AuxiliaryCell::Second) {
        _canvas_panel.set_game(&_game);
        _control_panel.set_game(&_game);
        _control_panel.set_canvas(&_canvas_panel.get_canvas());

        _canvas_panel._on_board_changed = [this]() {
            _control_panel.on_board_changed();
            sync_buttons();   // the AI stops itself at game over
        };

        _control_panel._on_status_update = [this]() {
            _canvas_panel.update_status();
        };

        _canvas_panel.set_mode_callback([this](bool is_ai) {
            set_mode(is_ai); });
        _canvas_panel.set_start_callback([this]() {
            _control_panel.reset_game();   // fresh board, then run
            _control_panel.start_ai();
            sync_buttons(); });
        _canvas_panel.set_stop_callback([this]() {
            _control_panel.stop_ai();
            sync_buttons(); });
        _canvas_panel.set_continue_callback([this]() {
            _control_panel.start_ai();
            sync_buttons(); });
        _canvas_panel.set_new_game_callback([this]() {
            _control_panel.reset_game();
            sync_buttons(); });

        _splitter.setContent(_canvas_panel, _control_panel);
        setLayout(&_splitter);
    }

    // switching mode always starts a fresh board, so a score is never carried
    // across from a run by the other player. The AI does NOT begin playing
    // here - that waits for Start.
    void set_mode(bool is_ai) {
        _canvas_panel.set_human_mode(!is_ai);
        _control_panel.set_human_mode(!is_ai);
        sync_buttons();
    }

private:
    void sync_buttons() {
        _canvas_panel.update_buttons(_canvas_panel.is_ai_mode(),
                                     _control_panel.is_ai_running(),
                                     _control_panel.can_continue());
    }
};
