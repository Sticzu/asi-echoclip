#pragma once

#include <gtkmm.h>
#include <memory>
#include <thread>
#include <atomic>
#include "audio_recorder.hpp"
#include "whisper_client.hpp"
#include "history_db.hpp"
#include "fft_visualizer.hpp"

class AppWindow : public Gtk::Window {
public:
    AppWindow();
    ~AppWindow() override;

protected:
    bool on_key_press_event(GdkEventKey* event) override;

private:
    // Core components
    AudioRecorder recorder_;
    WhisperClient client_;
    HistoryDb db_;
    std::string temp_wav_path_;

    // Layout
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Notebook notebook_;

    // Status bar header
    Gtk::Box header_box_{Gtk::ORIENTATION_HORIZONTAL, 12};
    Gtk::Label server_status_label_;
    Gtk::Button refresh_server_btn_{"🔄 Sprawdź serwer"};

    // Tab 1: Nagrywanie
    Gtk::Box rec_box_{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Label timer_label_{"00:00"};
    FftVisualizerWidget visualizer_;
    Gtk::Button record_btn_{"🎙️ Rozpocznij nagrywanie (Spacja)"};
    Gtk::Label action_status_label_{""};
    Gtk::ScrolledWindow text_scrolled_;
    Gtk::TextView text_view_;
    Glib::RefPtr<Gtk::TextBuffer> text_buffer_;

    Gtk::Box btn_box_{Gtk::ORIENTATION_HORIZONTAL, 8};
    Gtk::Button copy_btn_{"📋 Kopiuj do schowka"};
    Gtk::Button play_btn_{"▶️ Odsłuchaj nagranie"};
    Gtk::Button clear_btn_{"🧹 Wyczyść"};

    // Tab 2: Historia
    Gtk::Box hist_box_{Gtk::ORIENTATION_VERTICAL, 8};
    Gtk::SearchEntry search_entry_;
    Gtk::ScrolledWindow hist_scrolled_;
    Gtk::TreeView tree_view_;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(col_id);
            add(col_timestamp);
            add(col_duration);
            add(col_preview);
            add(col_full_text);
        }
        Gtk::TreeModelColumn<int64_t> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_timestamp;
        Gtk::TreeModelColumn<Glib::ustring> col_duration;
        Gtk::TreeModelColumn<Glib::ustring> col_preview;
        Gtk::TreeModelColumn<Glib::ustring> col_full_text;
    };

    ModelColumns columns_;
    Glib::RefPtr<Gtk::ListStore> list_store_;

    Gtk::ScrolledWindow hist_preview_scrolled_;
    Gtk::TextView hist_preview_view_;
    Glib::RefPtr<Gtk::TextBuffer> hist_preview_buffer_;

    Gtk::Box hist_btn_box_{Gtk::ORIENTATION_HORIZONTAL, 8};
    Gtk::Button hist_copy_btn_{"📋 Kopiuj wybrany"};
    Gtk::Button hist_delete_btn_{"🗑️ Usuń wybrany"};
    Gtk::Button hist_clear_btn_{"💥 Wyczyść całą historię"};

    // Timer & Dispatcher for background worker
    sigc::connection timer_conn_;
    Glib::Dispatcher dispatcher_transcribe_done_;
    Glib::Dispatcher dispatcher_server_checked_;

    std::atomic<bool> is_processing_{false};
    bool worker_success_{false};
    std::string worker_result_text_;
    double last_recorded_duration_{0.0};
    bool server_is_online_{false};

    // UI Setup & Handlers
    void setup_ui();
    void load_styles();
    void update_server_status();
    void on_record_clicked();
    void start_recording();
    void stop_and_transcribe();
    bool on_timer_tick();
    void on_transcription_finished();

    void on_copy_clicked();
    void on_play_clicked();
    void on_clear_clicked();

    void refresh_history(const std::string& query = "");
    void on_search_changed();
    void on_history_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_history_selection_changed();
    void on_hist_copy_clicked();
    void on_hist_delete_clicked();
    void on_hist_clear_clicked();
};
