#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

class FftAnalyzer {
public:
    using Complex = std::complex<float>;
    static constexpr size_t FFT_SIZE = 512;
    static constexpr float PI = 3.14159265358979323846f;

    static void compute_fft(std::vector<Complex>& a) {
        size_t n = a.size();
        if (n <= 1) return;

        for (size_t i = 1, j = 0; i < n; i++) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }

        for (size_t len = 2; len <= n; len <<= 1) {
            float ang = -2.0f * PI / static_cast<float>(len);
            Complex wlen(std::cos(ang), std::sin(ang));
            for (size_t i = 0; i < n; i += len) {
                Complex w(1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; j++) {
                    Complex u = a[i + j];
                    Complex v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    // Oblicza pasma częstotliwości (np. num_bars = 36) z próbek audio PCM
    static std::vector<float> compute_spectrum(const std::vector<float>& samples, size_t num_bars = 36) {
        std::vector<float> bars(num_bars, 0.0f);
        if (samples.empty()) return bars;

        std::vector<Complex> buffer(FFT_SIZE, Complex(0.0f, 0.0f));
        size_t count = std::min(samples.size(), FFT_SIZE);

        // Zastosowanie okna Hanninga dla eliminacji artefaktów krawędziowych
        for (size_t i = 0; i < count; ++i) {
            float hanning = 0.5f * (1.0f - std::cos(2.0f * PI * i / (count - 1)));
            buffer[i] = Complex(samples[i] * hanning, 0.0f);
        }

        compute_fft(buffer);

        // Połowa widma (Nyquist): 256 binów dla próbkowania 16kHz to 0 - 8000 Hz.
        // Głos ludzki to głównie pasmo 80 Hz do 3500 Hz (biny od ~3 do ~115)
        size_t min_bin = 2;
        size_t max_bin = 120; // ~3750 Hz
        if (max_bin > FFT_SIZE / 2) max_bin = FFT_SIZE / 2;

        float bin_step = static_cast<float>(max_bin - min_bin) / static_cast<float>(num_bars);

        for (size_t b = 0; b < num_bars; ++b) {
            size_t start = min_bin + static_cast<size_t>(b * bin_step);
            size_t end = min_bin + static_cast<size_t>((b + 1) * bin_step);
            if (end <= start) end = start + 1;
            if (end > max_bin) end = max_bin;

            float sum_mag = 0.0f;
            for (size_t k = start; k < end; ++k) {
                float mag = std::abs(buffer[k]);
                sum_mag += mag;
            }
            float avg_mag = sum_mag / (end - start);

            // Normalizacja i nieliniowe wzmocnienie logarytmiczne dla ładnej dynamiki wizualnej
            float val = std::log10(1.0f + avg_mag * 15.0f) / 1.8f;
            bars[b] = std::clamp(val, 0.0f, 1.0f);
        }

        return bars;
    }
};
