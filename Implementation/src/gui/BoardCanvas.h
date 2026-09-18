#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Key.h>
#include <gui/Application.h>
#include "core/game2048.h"

class BoardCanvas : public gui::Canvas {
private:
    Game2048* _p_game = nullptr;

    static constexpr float _cell_size = 100.0f;
    static constexpr float _cell_gap = 8.0f;
    static constexpr float _board_padding = 12.0f;
    static constexpr float _corner_radius = 8.0f;
    bool _evil_mode = false;

    static td::ColorID get_tile_color(int value) {
        bool dark = gui::Application::isDarkMode();
        switch(value) {
            case 2:    return dark ? td::ColorID::NavajoWhite  : td::ColorID::PeachPuff;
            case 4:    return dark ? td::ColorID::SandyBrown   : td::ColorID::Moccasin;
            case 8:    return td::ColorID::LightSalmon;
            case 16:   return td::ColorID::Salmon;
            case 32:   return td::ColorID::Coral;
            case 64:   return td::ColorID::OrangeRed;
            case 128:  return td::ColorID::Gold;
            case 256:  return td::ColorID::DarkOrange;
            case 512:  return td::ColorID::Orange;
            case 1024: return td::ColorID::Tangerine;
            case 2048: return td::ColorID::Crimson;
            default:   return td::ColorID::Firebrick;
        }
    }
    static td::ColorID get_text_color(int value) {
        if (value <= 4) {
            bool dark = gui::Application::isDarkMode();
            if (value == 2) return dark ? td::ColorID::Black : td::ColorID::DimGray;
            return dark ? td::ColorID::White : td::ColorID::DimGray;
        }
        return td::ColorID::White;
    }
    static gui::Font::ID get_font_for_value(int value) {
        if (value >= 1000) {
            return gui::Font::ID::SystemLargerBold;
        }
        return gui::Font::ID::SystemLargestBold;
    }
protected:
    void onDraw(const gui::Rect& rect) override {
        if (!_p_game) {
            return;
        }
        gui::Size size;
        getSize(size);

        constexpr float margin = 24.0f;
        float total_size = 4 * _cell_size + 5 * _cell_gap + 2 * _board_padding;
        float available_w = size.width - 2.0f * margin;
        float available_h = size.height - 2.0f * margin;
        float side = available_w < available_h ? available_w : available_h;
        float scale = side / total_size;
        float offset_x = (size.width - side) / 2.0f;
        float offset_y = (size.height - side) / 2.0f;

        bool darkMode = gui::Application::isDarkMode();
        if (darkMode) {
            gui::Shape bg;
            bg.createRect(rect);
            bg.drawFill(td::ColorID::Black);
        }

        gui::Rect board_rect(offset_x, offset_y, offset_x + side, offset_y + side);
        gui::Shape board_shape;
        board_shape.createRoundedRect(board_rect, _corner_radius * scale);
        board_shape.drawFill(darkMode ? td::ColorID::Chocolate : td::ColorID::BurlyWood);

        for (int r = 0; r < 4; ++r) {  // calculate where each tile goes on screen
            for (int c = 0; c < 4; ++c) {
                float x = offset_x + (_board_padding + _cell_gap + c * (_cell_size + _cell_gap)) * scale;
                float y = offset_y + (_board_padding + _cell_gap + r * (_cell_size + _cell_gap)) * scale;
                float cs = _cell_size * scale;
                gui::Rect cell_rect(x, y, x + cs, y + cs);
                gui::Shape cell_shape;

                int rank = get_tile(_p_game->board, r * 4 + c);
                int val = rank ? (1 << rank) : 0;
                if (val == 0) {
                    cell_shape.createRoundedRect(cell_rect, _corner_radius * scale);
                    cell_shape.drawFill(darkMode ? td::ColorID::Sienna : td::ColorID::Tan);
                } else {
                    cell_shape.createRoundedRect(cell_rect, _corner_radius * scale);
                    cell_shape.drawFill(get_tile_color(val));

                    td::String num_str;
                    num_str.format("%d", val);
                    gui::DrawableString::draw(num_str, cell_rect, 
                        get_font_for_value(val), get_text_color(val), td::TextAlignment::Center, td::VAlignment::Center);
                }
            }
        }
    }
    bool onKeyPressed(const gui::Key& key) override {
        if (!_p_game || key.getType() != gui::Key::Type::Virtual) {
            return false;
        }
        if (!_is_human_mode) {
            return true;
        }
        Direction dir;
        switch(key.getVirtual()) {
            case gui::Key::Virtual::Up:    dir = Direction::UP;    break;
            case gui::Key::Virtual::Down:  dir = Direction::DOWN;  break;
            case gui::Key::Virtual::Left:  dir = Direction::LEFT;  break;
            case gui::Key::Virtual::Right: dir = Direction::RIGHT; break;
            default: return false;
        }     
        if (_p_game->is_game_over()) {
            return true;
        }
        if (_p_game->make_move(dir)) {
            if (_evil_mode) {
                _p_game->spawn_evil_tile();
            } else {
                _p_game->spawn_tile();
            }
            reDraw();  // game changed
            if (_on_board_changed) {
                _on_board_changed();
            }
        }
        return true;
    }
public:
    std::function<void()> _on_board_changed;
    bool _is_human_mode = true;
    
    void set_human_mode(bool v) {
        _is_human_mode = v;
    }
    void set_evil_mode(bool v) {
        _evil_mode = v;
    }
    BoardCanvas() : gui::Canvas({gui::InputDevice::Event::Keyboard}) {}

    void set_game(Game2048* p_game) {
        _p_game = p_game;
    }
    Game2048* get_game() const {
        return _p_game;
    }
    float get_board_size() const {
        return 4 * _cell_size + 5 * _cell_gap + 2 * _board_padding;
    }
    void refresh() { reDraw(); }
};