#pragma once
#include <gui/MenuBar.h>

class MenuBar : public gui::MenuBar {
private:
    gui::SubMenu _sub_app;

protected:
    void populate_sub_app_menu() {
        auto& items = _sub_app.getItems();
        items[0].initAsActionItem("About 2048 AI", 10);
        items[1].initAsQuitAppActionItem("Quit", "q");
    }

public:
    MenuBar() : gui::MenuBar(1), _sub_app(10, "App", 2)
    {
        populate_sub_app_menu();
        _menus[0] = &_sub_app;
    }
};
