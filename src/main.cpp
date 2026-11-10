#include <gtkmm.h>
#include <iostream>
#include "app_window.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.sticzu.asi_echoclip");

    AppWindow window;
    // Ustawienie nazwy klasy dla reguł okna w Hyprland
    window.set_wmclass("asi-echoclip", "asi-echoclip");

    return app->run(window);
}
