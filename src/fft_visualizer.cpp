#include "fft_visualizer.hpp"
#include "fft_analyzer.hpp"
#include <algorithm>
#include <cmath>

FftVisualizerWidget::FftVisualizerWidget() 
    : current_bars_(NUM_BARS, 0.0f), peak_bars_(NUM_BARS, 0.0f) {
    set_size_request(-1, 85);
}

void FftVisualizerWidget::reset() {
    std::fill(current_bars_.begin(), current_bars_.end(), 0.0f);
    std::fill(peak_bars_.begin(), peak_bars_.end(), 0.0f);
    is_recording_ = false;
    queue_draw();
}

void FftVisualizerWidget::update_samples(const std::vector<float>& samples, bool is_recording) {
    is_recording_ = is_recording;

    if (is_recording) {
        auto target_bars = FftAnalyzer::compute_spectrum(samples, NUM_BARS);

        // Płynna interpolacja i opadanie słupków (smooth decay)
        for (size_t i = 0; i < NUM_BARS; ++i) {
            float target = target_bars[i];
            if (target > current_bars_[i]) {
                current_bars_[i] = target; // Szybki wzrost
            } else {
                current_bars_[i] = current_bars_[i] * 0.78f + target * 0.22f; // Płynne opadanie
            }

            // Opadanie znaczników szczytowych (peaks)
            if (current_bars_[i] > peak_bars_[i]) {
                peak_bars_[i] = current_bars_[i];
            } else {
                peak_bars_[i] = std::max(0.0f, peak_bars_[i] - 0.035f);
            }
        }
    } else {
        // Stopniowe wygaszanie po zakończeniu nagrywania
        for (size_t i = 0; i < NUM_BARS; ++i) {
            current_bars_[i] *= 0.8f;
            peak_bars_[i] = std::max(0.0f, peak_bars_[i] - 0.05f);
        }
        idle_phase_ += 0.05f;
    }

    queue_draw();
}

bool FftVisualizerWidget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const double width = allocation.get_width();
    const double height = allocation.get_height();

    // 1. Tło wizualizatora - zaokrąglony prostokąt, ciemne estetyczne tło
    cr->save();
    double radius = 10.0;
    cr->begin_new_sub_path();
    cr->arc(width - radius, radius, radius, -M_PI / 2, 0);
    cr->arc(width - radius, height - radius, radius, 0, M_PI / 2);
    cr->arc(radius, height - radius, radius, M_PI / 2, M_PI);
    cr->arc(radius, radius, radius, M_PI, 3 * M_PI / 2);
    cr->close_path();

    cr->set_source_rgba(0.09, 0.09, 0.15, 0.95); // #171726
    cr->fill_preserve();
    cr->set_source_rgba(0.25, 0.26, 0.38, 0.6); // Border
    cr->set_line_width(1.2);
    cr->stroke();
    cr->restore();

    // 2. Siatka pomocnicza w tle (delikatne linie poziome)
    cr->save();
    cr->set_line_width(0.8);
    cr->set_source_rgba(0.2, 0.22, 0.32, 0.35);
    for (double y = height * 0.25; y < height; y += height * 0.25) {
        cr->move_to(12, y);
        cr->line_to(width - 12, y);
        cr->stroke();
    }
    cr->restore();

    // 3. Rysowanie słupków częstotliwości Fouriera
    double padding_x = 16.0;
    double padding_bottom = 8.0;
    double padding_top = 16.0;
    double avail_width = width - (padding_x * 2.0);
    double avail_height = height - padding_bottom - padding_top;

    double bar_spacing = 3.0;
    double bar_width = (avail_width - ((NUM_BARS - 1) * bar_spacing)) / NUM_BARS;
    if (bar_width < 1.0) bar_width = 1.0;

    for (size_t i = 0; i < NUM_BARS; ++i) {
        double x = padding_x + i * (bar_width + bar_spacing);
        double val = current_bars_[i];

        // Minimalna widoczność słupków (baseline)
        if (val < 0.02) val = 0.02;

        double bar_h = val * avail_height;
        double y = height - padding_bottom - bar_h;

        // Gradient słupka (od dołu: niebieski/turkus -> do góry: fiolet/róż)
        auto gradient = Cairo::LinearGradient::create(x, height - padding_bottom, x, y);
        if (is_recording_) {
            gradient->add_color_stop_rgba(0.0, 0.1, 0.7, 0.9, 0.9);   // Turkus / błękit
            gradient->add_color_stop_rgba(0.6, 0.6, 0.3, 0.95, 0.95); // Fiolet
            gradient->add_color_stop_rgba(1.0, 0.95, 0.2, 0.45, 1.0);  // Róż / czerwień
        } else {
            gradient->add_color_stop_rgba(0.0, 0.2, 0.3, 0.5, 0.5);
            gradient->add_color_stop_rgba(1.0, 0.4, 0.3, 0.6, 0.6);
        }

        cr->save();
        cr->set_source(gradient);
        // Zaokrąglony słupek
        double r = std::min(bar_width / 2.0, 3.0);
        cr->begin_new_sub_path();
        cr->arc(x + bar_width - r, y + r, r, -M_PI / 2, 0);
        cr->line_to(x + bar_width, height - padding_bottom);
        cr->line_to(x, height - padding_bottom);
        cr->arc(x + r, y + r, r, M_PI, 3 * M_PI / 2);
        cr->close_path();
        cr->fill();
        cr->restore();

        // Znacznik szczytowy (Peak indicator cap)
        if (peak_bars_[i] > 0.04) {
            double peak_y = height - padding_bottom - (peak_bars_[i] * avail_height);
            cr->save();
            cr->set_source_rgba(0.98, 0.85, 0.4, 0.9); // Złoty punkt szczytowy
            cr->set_line_width(1.5);
            cr->move_to(x, peak_y);
            cr->line_to(x + bar_width, peak_y);
            cr->stroke();
            cr->restore();
        }
    }

    // 4. Etykieta statusu mikrofonu w narożniku
    cr->save();
    if (is_recording_) {
        // Czerwona pulsująca kropka i tekst "🔴 MIKROFON NAGRYWA (FFT)"
        cr->set_source_rgb(0.95, 0.25, 0.35);
        cr->arc(26, 16, 4.5, 0, 2 * M_PI);
        cr->fill();

        cr->select_font_face("monospace", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
        cr->set_font_size(10.0);
        cr->set_source_rgba(0.95, 0.95, 0.95, 0.9);
        cr->move_to(36, 19.5);
        cr->show_text("MIKROFON AKTYWNY (WIDMO CZĘSTOTLIWOŚCI FOURIERA)");
    } else {
        cr->set_source_rgba(0.5, 0.55, 0.65, 0.6);
        cr->arc(26, 16, 3.5, 0, 2 * M_PI);
        cr->fill();

        cr->select_font_face("sans-serif", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
        cr->set_font_size(10.0);
        cr->set_source_rgba(0.5, 0.55, 0.65, 0.7);
        cr->move_to(36, 19.5);
        cr->show_text("Wizualizator widma audio (gotowy)");
    }
    cr->restore();

    return true;
}
