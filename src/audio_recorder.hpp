#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/types.h>

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    bool start_recording(const std::string& output_wav_path);
    bool stop_recording();
    bool is_recording() const { return is_recording_; }
    double get_elapsed_seconds() const;
    std::string get_current_wav_path() const { return current_wav_path_; }

    // Pobiera ostatnie próbki dźwiękowe (znormalizowane od -1.0 do 1.0) dla FFT
    void get_latest_samples(std::vector<float>& out_samples, size_t count);

private:
    std::atomic<bool> is_recording_{false};
    pid_t recorder_pid_{-1};
    int pipe_fd_{-1};
    std::string current_wav_path_;
    std::chrono::steady_clock::time_point start_time_;

    std::thread reader_thread_;
    std::mutex audio_mutex_;
    std::vector<int16_t> recorded_samples_;
    std::vector<float> ring_samples_;
    size_t ring_head_{0};

    std::string detect_pipewire_source_id();
    void reader_worker();
    bool write_wav_file(const std::string& path, const std::vector<int16_t>& samples, int sample_rate = 16000);
};
