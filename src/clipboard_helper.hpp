#pragma once

#include <string>
#include <gtkmm.h>
#include <gdk/gdk.h>
#include <unistd.h>
#include <sys/wait.h>

class ClipboardHelper {
public:
    static bool copy_to_clipboard(const std::string& text) {
        if (text.empty()) return false;

        // 1. GTK Clipboard (Wayland oraz X11/XFCE)
        try {
            auto clip = Gtk::Clipboard::get(GDK_SELECTION_CLIPBOARD);
            if (clip) {
                clip->set_text(text);
                clip->store(); // Wymusza utrwalenie w menedżerze schowka X11/XFCE
            }
            auto primary = Gtk::Clipboard::get(GDK_SELECTION_PRIMARY);
            if (primary) {
                primary->set_text(text);
            }
        } catch (...) {}

        // Funkcja pomocnicza do bezpiecznego przekazywania tekstu do narzędzi CLI
        auto send_to_tool = [&text](const char* cmd, const char* arg1 = nullptr, const char* arg2 = nullptr, const char* arg3 = nullptr) {
            int pipefd[2];
            if (pipe(pipefd) != 0) return;
            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                if (arg3) execlp(cmd, cmd, arg1, arg2, arg3, nullptr);
                else if (arg2) execlp(cmd, cmd, arg1, arg2, nullptr);
                else if (arg1) execlp(cmd, cmd, arg1, nullptr);
                else execlp(cmd, cmd, nullptr);
                _exit(127);
            }
            close(pipefd[0]);
            ssize_t w = write(pipefd[1], text.data(), text.size());
            (void)w;
            close(pipefd[1]);
            int status = 0;
            waitpid(pid, &status, 0);
        };

        // 2. Wayland (Hyprland / Sway)
        if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
            send_to_tool("wl-copy");
            send_to_tool("wl-copy", "--primary");
        }

        // 3. X11 / XFCE4
        if (std::getenv("DISPLAY") != nullptr) {
            send_to_tool("xclip", "-selection", "clipboard");
            send_to_tool("xclip", "-selection", "primary");
            send_to_tool("xsel", "-b", "-i");
            send_to_tool("xsel", "-p", "-i");
        }

        return true;
    }
};
