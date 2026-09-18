#pragma once
#include <gui/Window.h>
#include "MenuBar.h"
#include "MainView.h"

class MainWindow : public gui::Window
{
private:
    MenuBar  _menu_bar;
    MainView _main_view;

public:
    MainWindow() : gui::Window(gui::Size(1050, 720)) {
        setResizable(false);
        setTitle("2048 AI");
        _menu_bar.setAsMain(this);
        setCentralView(&_main_view);
    }
    bool shouldClose() override {
        return true;
    }
    void onClose() override {
        gui::Window::onClose();
    }

    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override {
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        if (menuID == 10 && actionID == 10) {
            return true; // about -> no-op
        }
        return false;
    }
};
