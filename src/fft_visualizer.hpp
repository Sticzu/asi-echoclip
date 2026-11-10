#pragma once

#include <gtkmm.h>
#include <vector>
#include <chrono>

class FftVisualizerWidget : public Gtk::DrawingArea {
public:
    FftVisualizerWidget();
    ~FftVisualizerWidget() override = default;

    void update_samples(const std::vector<float>& samples, bool is_recording);
    void reset();

protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

private:
    static constexpr size_t NUM_BARS = 40;
    std::vector<float> current_bars_;
    std::vector<float> peak_bars_;
    bool is_recording_{false};
    float idle_phase_{0.0f};
};
