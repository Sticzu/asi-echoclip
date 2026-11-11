#include "app_window.hpp"
#include "clipboard_helper.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <gdk/gdkkeysyms.h>

AppWindow::AppWindow() {
    set_title("asi-echoclip");
    set_default_size(720, 560);
    set_position(Gtk::WIN_POS_CENTER);
    set_border_width(12);

    temp_wav_path_ = "/tmp/whisper_voice_clipboard.wav";

    db_.init();
    load_styles();
    setup_ui();

    dispatcher_transcribe_done_.connect(sigc::mem_fun(*this, &AppWindow::on_transcription_finished));
    dispatcher_server_checked_.connect([this]() {
        if (server_is_online_) {
            server_status_label_.set_markup("<span foreground='#a6e3a1'>● Whisper (" + client_.get_active_host_display() + ") Połączony</span>");
        } else {
            server_status_label_.set_markup("<span foreground='#f38ba8'>● Whisper (" + client_.get_base_url() + ") Brak połączenia</span>");
        }
    });

    update_server_status();
    refresh_history();
}

AppWindow::~AppWindow() {
    if (recorder_.is_recording()) {
        recorder_.stop_recording();
    }
}

void AppWindow::load_styles() {
    auto css_provider = Gtk::CssProvider::create();
    std::vector<std::string> candidates = {
        "resources/style.css",
        "/usr/local/share/asi-echoclip/style.css",
        "/usr/share/asi-echoclip/style.css",
        "/home/sticzu/projekty/asi-echoclip/resources/style.css",
        "/home/sticzu/projekty/whisper-clipboard/resources/style.css"
    };

    // Sprawdź ścieżkę względem pliku wykonywalnego
    char exe_buf[1024];
    ssize_t len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);
    if (len > 0) {
        exe_buf[len] = '\0';
        std::filesystem::path exe_dir = std::filesystem::path(exe_buf).parent_path();
        candidates.insert(candidates.begin(), (exe_dir / "resources" / "style.css").string());
        candidates.insert(candidates.begin() + 1, (exe_dir.parent_path() / "share" / "asi-echoclip" / "style.css").string());
    }

    std::string found_path;
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) {
            found_path = p;
            break;
        }
    }

    try {
        if (!found_path.empty()) {
            css_provider->load_from_path(found_path);
        } else {
            // Wbudowany fallback CSS
            std::string fallback_css =
                "window { background-color: #1e1e2e; color: #cdd6f4; }"
                "button { background-color: #313244; color: #cdd6f4; border-radius: 8px; padding: 8px 16px; }"
                ".btn-record-idle { background-color: #1e66f5; color: white; font-weight: bold; border-radius: 12px; padding: 14px; }"
                ".btn-record-active { background-color: #d20f39; color: white; font-weight: bold; border-radius: 12px; padding: 14px; }";
            css_provider->load_from_data(fallback_css);
        }
        auto screen = Gdk::Screen::get_default();
        if (screen) {
            Gtk::StyleContext::add_provider_for_screen(
                screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    } catch (const Glib::Error& ex) {
        std::cerr << "Błąd ładowania CSS: " << ex.what() << std::endl;
    }

}

void AppWindow::setup_ui() {
    add(main_box_);

    // 1. Górny pasek statusu Whisper.cpp
    server_status_label_.set_markup("<span>● Sprawdzanie serwera Whisper...</span>");
    server_status_label_.set_halign(Gtk::ALIGN_START);
    server_status_label_.set_hexpand(true);

    refresh_server_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::update_server_status));
    header_box_.pack_start(server_status_label_, Gtk::PACK_EXPAND_WIDGET);
    header_box_.pack_end(refresh_server_btn_, Gtk::PACK_SHRINK);
    main_box_.pack_start(header_box_, Gtk::PACK_SHRINK);

    // 2. Notebook (Zakładki)
    main_box_.pack_start(notebook_, Gtk::PACK_EXPAND_WIDGET);

    // ================= TAB 1: NAGRYWANIE =================
    timer_label_.get_style_context()->add_class("timer-label");
    timer_label_.set_halign(Gtk::ALIGN_CENTER);
    rec_box_.pack_start(timer_label_, Gtk::PACK_SHRINK);
    rec_box_.pack_start(visualizer_, Gtk::PACK_SHRINK);

    record_btn_.get_style_context()->add_class("btn-record-idle");
    record_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_record_clicked));
    rec_box_.pack_start(record_btn_, Gtk::PACK_SHRINK);

    action_status_label_.set_halign(Gtk::ALIGN_CENTER);
    rec_box_.pack_start(action_status_label_, Gtk::PACK_SHRINK);

    text_buffer_ = Gtk::TextBuffer::create();
    text_view_.set_buffer(text_buffer_);
    text_view_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    text_view_.set_hexpand(true);
    text_view_.set_vexpand(true);

    text_scrolled_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    text_scrolled_.add(text_view_);
    text_scrolled_.set_vexpand(true);
    rec_box_.pack_start(text_scrolled_, Gtk::PACK_EXPAND_WIDGET);

    copy_btn_.get_style_context()->add_class("btn-copy");
    copy_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_copy_clicked));
    play_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_play_clicked));
    clear_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_clear_clicked));

    btn_box_.pack_start(copy_btn_, Gtk::PACK_EXPAND_WIDGET);
    btn_box_.pack_start(play_btn_, Gtk::PACK_EXPAND_WIDGET);
    btn_box_.pack_start(clear_btn_, Gtk::PACK_SHRINK);
    rec_box_.pack_start(btn_box_, Gtk::PACK_SHRINK);

    notebook_.append_page(rec_box_, "🎙️ Nagrywanie i Transkrypcja");

    // ================= TAB 2: HISTORIA =================
    search_entry_.set_placeholder_text("🔍 Szukaj w historii transkrypcji...");
    search_entry_.signal_search_changed().connect(sigc::mem_fun(*this, &AppWindow::on_search_changed));
    hist_box_.pack_start(search_entry_, Gtk::PACK_SHRINK);

    list_store_ = Gtk::ListStore::create(columns_);
    tree_view_.set_model(list_store_);
    tree_view_.append_column("Czas", columns_.col_timestamp);
    tree_view_.append_column("Długość", columns_.col_duration);
    tree_view_.append_column("Transkrypcja", columns_.col_preview);

    tree_view_.get_column(0)->set_resizable(true);
    tree_view_.get_column(1)->set_resizable(true);
    tree_view_.get_column(2)->set_resizable(true);
    tree_view_.get_column(2)->set_expand(true);

    tree_view_.signal_row_activated().connect(sigc::mem_fun(*this, &AppWindow::on_history_row_activated));
    tree_view_.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &AppWindow::on_history_selection_changed));

    hist_scrolled_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    hist_scrolled_.add(tree_view_);
    hist_scrolled_.set_vexpand(true);
    hist_box_.pack_start(hist_scrolled_, Gtk::PACK_EXPAND_WIDGET);

    hist_preview_buffer_ = Gtk::TextBuffer::create();
    hist_preview_view_.set_buffer(hist_preview_buffer_);
    hist_preview_view_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    hist_preview_view_.set_editable(false);

    hist_preview_scrolled_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    hist_preview_scrolled_.add(hist_preview_view_);
    hist_preview_scrolled_.set_size_request(-1, 80);
    hist_box_.pack_start(hist_preview_scrolled_, Gtk::PACK_SHRINK);

    hist_copy_btn_.get_style_context()->add_class("btn-copy");
    hist_copy_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_hist_copy_clicked));
    hist_delete_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_hist_delete_clicked));
    hist_clear_btn_.signal_clicked().connect(sigc::mem_fun(*this, &AppWindow::on_hist_clear_clicked));

    hist_btn_box_.pack_start(hist_copy_btn_, Gtk::PACK_EXPAND_WIDGET);
    hist_btn_box_.pack_start(hist_delete_btn_, Gtk::PACK_EXPAND_WIDGET);
    hist_btn_box_.pack_end(hist_clear_btn_, Gtk::PACK_SHRINK);
    hist_box_.pack_start(hist_btn_box_, Gtk::PACK_SHRINK);

    notebook_.append_page(hist_box_, "📜 Historia transkrypcji");

    show_all_children();
}

void AppWindow::update_server_status() {
    std::thread([this]() {
        server_is_online_ = client_.check_server_online();
        dispatcher_server_checked_.emit();
    }).detach();
}

bool AppWindow::on_key_press_event(GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) {
        close();
        return true;
    }

    if (event->keyval == GDK_KEY_space) {
        // Jeśli użytkownik skupia się na edycji tekstu lub wyszukiwarce, spacja powinna wpisywać znak
        if (!text_view_.has_focus() && !search_entry_.has_focus()) {
            on_record_clicked();
            return true;
        }
    }

    return Gtk::Window::on_key_press_event(event);
}

void AppWindow::on_record_clicked() {
    if (is_processing_) {
        return;
    }

    if (!recorder_.is_recording()) {
        start_recording();
    } else {
        stop_and_transcribe();
    }
}

void AppWindow::start_recording() {
    if (!recorder_.start_recording(temp_wav_path_)) {
        action_status_label_.set_markup("<span foreground='#f38ba8'>❌ Błąd uruchamiania nagrywania audio</span>");
        return;
    }

    action_status_label_.set_markup("<span foreground='#89dceb'>🎙️ Trwa nagrywanie... Powiedz coś i kliknij ponownie</span>");

    record_btn_.set_label("⏹️ ZATZYMAJ I TRANSKRYBUJ (Spacja)");
    record_btn_.get_style_context()->remove_class("btn-record-idle");
    record_btn_.get_style_context()->add_class("btn-record-active");

    timer_conn_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &AppWindow::on_timer_tick), 33);
}

bool AppWindow::on_timer_tick() {
    if (!recorder_.is_recording()) {
        return false;
    }

    double elapsed = recorder_.get_elapsed_seconds();
    int total_secs = static_cast<int>(elapsed);
    int mins = total_secs / 60;
    int secs = total_secs % 60;
    int millis = static_cast<int>((elapsed - total_secs) * 10);

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs << "." << millis;
    timer_label_.set_text(ss.str());

    // Pobierz próbki i zaktualizuj wizualizator FFT w czasie rzeczywistym
    std::vector<float> samples;
    recorder_.get_latest_samples(samples, 512);
    visualizer_.update_samples(samples, true);

    return true;
}

void AppWindow::stop_and_transcribe() {
    timer_conn_.disconnect();
    visualizer_.reset();
    last_recorded_duration_ = recorder_.get_elapsed_seconds();

    bool stop_ok = recorder_.stop_recording();
    if (!stop_ok) {
        record_btn_.set_label("🎙️ Rozpocznij nagrywanie (Spacja)");
        record_btn_.get_style_context()->remove_class("btn-record-active");
        record_btn_.get_style_context()->add_class("btn-record-idle");
        action_status_label_.set_markup("<span foreground='#f38ba8'>⚠️ Nagranie było zbyt krótkie lub puste</span>");
        return;
    }

    is_processing_ = true;
    record_btn_.set_label("⏳ PRZETWARZANIE PRZEZ WHISPER.CPP (:8999)...");
    record_btn_.get_style_context()->remove_class("btn-record-active");
    record_btn_.get_style_context()->add_class("btn-record-processing");
    action_status_label_.set_markup("<span foreground='#f9e2af'>⏳ Wysyłanie audio do Whisper.cpp...</span>");

    // Transkrypcja w osobnym wątku
    std::thread([this]() {
        auto res = client_.transcribe(temp_wav_path_);
        worker_success_ = res.first;
        worker_result_text_ = res.second;
        is_processing_ = false;
        dispatcher_transcribe_done_.emit();
    }).detach();
}

void AppWindow::on_transcription_finished() {
    record_btn_.set_label("🎙️ Rozpocznij nagrywanie (Spacja)");
    record_btn_.get_style_context()->remove_class("btn-record-processing");
    record_btn_.get_style_context()->add_class("btn-record-idle");

    if (!worker_success_) {
        action_status_label_.set_markup("<span foreground='#f38ba8'>❌ " + worker_result_text_ + "</span>");
        return;
    }

    if (worker_result_text_.empty()) {
        action_status_label_.set_markup("<span foreground='#f9e2af'>⚠️ Cisza / Nie rozpoznano mowy w nagraniu</span>");
        return;
    }

    // Wstaw wynik do pola edycyjnego
    text_buffer_->set_text(worker_result_text_);

    // Skopiuj automatycznie do schowka
    ClipboardHelper::copy_to_clipboard(worker_result_text_);
    action_status_label_.set_markup("<span foreground='#a6e3a1'>✅ Transkrybowano i automatycznie skopiowano do schowka!</span>");

    // Zapisz do bazy danych historii
    db_.add_entry(worker_result_text_, last_recorded_duration_);
    refresh_history();
}

void AppWindow::on_copy_clicked() {
    std::string text = text_buffer_->get_text();
    if (text.empty()) {
        action_status_label_.set_markup("<span foreground='#f9e2af'>⚠️ Pole tekstowe jest puste</span>");
        return;
    }
    ClipboardHelper::copy_to_clipboard(text);
    action_status_label_.set_markup("<span foreground='#a6e3a1'>✅ Skopiowano do schowka!</span>");
}

void AppWindow::on_play_clicked() {
    if (!std::filesystem::exists(temp_wav_path_)) {
        action_status_label_.set_markup("<span foreground='#f9e2af'>⚠️ Brak nagrania audio do odsłuchania</span>");
        return;
    }

    // Odtwórz plik za pomocą pw-play, paplay, aplay lub play w tle (kompatybilne z PipeWire, PulseAudio i ALSA)
    std::string cmd = "(pw-play \"" + temp_wav_path_ + "\" || paplay \"" + temp_wav_path_ + "\" || aplay \"" + temp_wav_path_ + "\" || play \"" + temp_wav_path_ + "\") >/dev/null 2>&1 &";
    int ret = system(cmd.c_str());
    (void)ret;
    action_status_label_.set_markup("<span foreground='#89b4fa'>▶️ Odtwarzanie nagrania...</span>");
}

void AppWindow::on_clear_clicked() {
    text_buffer_->set_text("");
    timer_label_.set_text("00:00");
    action_status_label_.set_text("");
}

void AppWindow::refresh_history(const std::string& query) {
    list_store_->clear();
    auto entries = db_.get_all_entries(query);

    for (const auto& entry : entries) {
        auto row = *(list_store_->append());
        row[columns_.col_id] = entry.id;
        row[columns_.col_timestamp] = entry.timestamp;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << entry.duration_sec << "s";
        row[columns_.col_duration] = ss.str();

        std::string preview = entry.text;
        // Zamień nowe linie na spacje w podglądzie
        for (char& c : preview) {
            if (c == '\n' || c == '\r') c = ' ';
        }
        if (preview.size() > 70) {
            preview = preview.substr(0, 67) + "...";
        }
        row[columns_.col_preview] = preview;
        row[columns_.col_full_text] = entry.text;
    }
}

void AppWindow::on_search_changed() {
    refresh_history(search_entry_.get_text());
}

void AppWindow::on_history_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /*column*/) {
    auto iter = list_store_->get_iter(path);
    if (iter) {
        auto row = *iter;
        Glib::ustring u_text = row[columns_.col_full_text];
        std::string text = u_text.raw();
        ClipboardHelper::copy_to_clipboard(text);
        action_status_label_.set_markup("<span foreground='#a6e3a1'>✅ Skopiowano z historii do schowka!</span>");
        text_buffer_->set_text(text);
        notebook_.set_current_page(0); // Przełącz na widok główny
    }
}

void AppWindow::on_history_selection_changed() {
    auto iter = tree_view_.get_selection()->get_selected();
    if (iter) {
        auto row = *iter;
        Glib::ustring u_text = row[columns_.col_full_text];
        std::string text = u_text.raw();
        hist_preview_buffer_->set_text(text);
    } else {
        hist_preview_buffer_->set_text("");
    }
}

void AppWindow::on_hist_copy_clicked() {
    auto iter = tree_view_.get_selection()->get_selected();
    if (iter) {
        auto row = *iter;
        Glib::ustring u_text = row[columns_.col_full_text];
        std::string text = u_text.raw();
        ClipboardHelper::copy_to_clipboard(text);
        action_status_label_.set_markup("<span foreground='#a6e3a1'>✅ Skopiowano do schowka!</span>");
    }
}

void AppWindow::on_hist_delete_clicked() {
    auto iter = tree_view_.get_selection()->get_selected();
    if (iter) {
        auto row = *iter;
        int64_t id = row[columns_.col_id];
        db_.delete_entry(id);
        refresh_history(search_entry_.get_text());
        hist_preview_buffer_->set_text("");
    }
}

void AppWindow::on_hist_clear_clicked() {
    Gtk::MessageDialog dialog(*this, "Czy na pewno chcesz wyczyścić całą historię transkrypcji?",
                              false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    if (dialog.run() == Gtk::RESPONSE_YES) {
        db_.clear_all();
        refresh_history();
        hist_preview_buffer_->set_text("");
    }
}
