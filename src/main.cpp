#include <gtkmm.h>
#include <iostream>
#include <string>
#include "app_window.hpp"
#include "whisper_client.hpp"

static void print_help(const char* prog) {
    std::cout << "asi-echoclip - Asystent mowy i schowka STT (Whisper.cpp)\n\n"
              << "Użycie: " << prog << " [OPCJE]\n\n"
              << "Opcje:\n"
              << "  -h, --help          Wyświetla tę pomoc i kończy działanie (bez wymogu X11)\n"
              << "  -v, --version       Wyświetla wersję programu (bez wymogu X11)\n"
              << "  -c, --check-server  Sprawdza połączenie z serwerem Whisper (port 8999)\n\n"
              << "Zmienne środowiskowe:\n"
              << "  WHISPER_SERVER_URL  Adres URL serwera Whisper (domyślnie: http://127.0.0.1:8999)\n"
              << "  GDK_BACKEND         Backend wyświetlania GTK (wayland lub x11)\n";
}

int main(int argc, char* argv[]) {
    // 1. Obsługa flag CLI przed inicjalizacją Gtk (działa także w trybie headless / bez DISPLAY)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "asi-echoclip v1.1.0 (Linux Mint / Debian / NixOS / XFCE4 / Wayland)\n";
            return 0;
        } else if (arg == "-c" || arg == "--check-server") {
            WhisperClient client;
            bool ok = client.check_server_online();
            if (ok) {
                std::cout << "OK: Whisper serwer (" << client.get_active_host_display() << ") jest aktywny i dostępny!\n";
                return 0;
            } else {
                std::cerr << "BŁĄD: Brak połączenia z serwerem Whisper pod adresem: " << client.get_base_url() << "\n";
                return 1;
            }
        }
    }

    // 2. Inicjalizacja aplikacji graficznej GTK3
    auto app = Gtk::Application::create(argc, argv, "org.sticzu.asi_echoclip");

    AppWindow window;
    window.set_wmclass("asi-echoclip", "asi-echoclip");

    return app->run(window);
}
