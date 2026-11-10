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

        // 1. GTK Clipboard
        try {
            auto clip = Gtk::Clipboard::get(GDK_SELECTION_CLIPBOARD);
            if (clip) {
                clip->set_text(text);
            }
        } catch (...) {}

        // 2. wl-copy (dla natywnego Wayland / Hyprland)
        int pipefd[2];
        if (pipe(pipefd) == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                execlp("wl-copy", "wl-copy", nullptr);
                _exit(127);
            }
            close(pipefd[0]);
            ssize_t w = write(pipefd[1], text.data(), text.size());
            (void)w;
            close(pipefd[1]);
            int status = 0;
            waitpid(pid, &status, 0);
        }

        return true;
    }
};
