#include "audio_recorder.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <filesystem>
#include <cstring>

AudioRecorder::AudioRecorder() : ring_samples_(1024, 0.0f) {}

AudioRecorder::~AudioRecorder() {
    if (is_recording_) {
        stop_recording();
    }
}

std::string AudioRecorder::detect_pipewire_source_id() {
    FILE* pipe = popen("wpctl status 2>/dev/null", "r");
    if (!pipe) return "";

    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    std::istringstream stream(output);
    std::string line;
    bool in_sources = false;
    std::string fallback_id;
    std::regex src_regex(R"((\*?)\s*(\d+)\.)");

    while (std::getline(stream, line)) {
        if (line.find("Sources:") != std::string::npos) {
            in_sources = true;
            continue;
        }
        if (in_sources) {
            if (line.find("Filters:") != std::string::npos || 
                line.find("Streams:") != std::string::npos || 
                line.find("Video") != std::string::npos) {
                break;
            }
            std::smatch match;
            if (std::regex_search(line, match, src_regex)) {
                bool is_default = (match[1].str() == "*");
                std::string id = match[2].str();
                if (is_default) {
                    return id;
                }
                if (fallback_id.empty()) {
                    fallback_id = id;
                }
            }
        }
    }

    return fallback_id;
}

bool AudioRecorder::start_recording(const std::string& output_wav_path) {
    if (is_recording_) {
        return false;
    }

    current_wav_path_ = output_wav_path;
    std::error_code ec;
    std::filesystem::remove(current_wav_path_, ec);

    {
        std::lock_guard<std::mutex> lock(audio_mutex_);
        recorded_samples_.clear();
        std::fill(ring_samples_.begin(), ring_samples_.end(), 0.0f);
        ring_head_ = 0;
    }

    std::string source_id = detect_pipewire_source_id();

    int fds[2];
    if (pipe(fds) != 0) {
        std::cerr << "Nie udało się utworzyć potoku pipe dla pw-record" << std::endl;
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        std::cerr << "Błąd fork() przy uruchamianiu pw-record" << std::endl;
        return false;
    }

    if (pid == 0) {
        // Proces potomny
        close(fds[0]); // Zamknij czytanie w potomku
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        freopen("/dev/null", "w", stderr);

        std::vector<std::string> args = {
            "pw-record",
            "--rate", "16000",
            "--channels", "1",
            "--format", "s16",
            "--raw",
            "-"
        };

        if (!source_id.empty()) {
            args.insert(args.begin() + 1, source_id);
            args.insert(args.begin() + 1, "--target");
        }

        std::vector<char*> c_args;
        for (auto& a : args) {
            c_args.push_back(const_cast<char*>(a.c_str()));
        }
        c_args.push_back(nullptr);

        execvp("pw-record", c_args.data());

        // Awaryjnie sox jeśli brak pw-record
        std::vector<char*> sox_args = {
            const_cast<char*>("sox"),
            const_cast<char*>("-d"),
            const_cast<char*>("-r"), const_cast<char*>("16000"),
            const_cast<char*>("-c"), const_cast<char*>("1"),
            const_cast<char*>("-b"), const_cast<char*>("16"),
            const_cast<char*>("-e"), const_cast<char*>("signed-integer"),
            const_cast<char*>("-t"), const_cast<char*>("raw"),
            const_cast<char*>("-"),
            nullptr
        };
        execvp("sox", sox_args.data());
        _exit(127);
    }

    close(fds[1]); // Zamknij zapis w rodzicu
    pipe_fd_ = fds[0];
    recorder_pid_ = pid;
    is_recording_ = true;
    start_time_ = std::chrono::steady_clock::now();

    reader_thread_ = std::thread(&AudioRecorder::reader_worker, this);
    return true;
}

void AudioRecorder::reader_worker() {
    int16_t buffer[512];
    while (is_recording_) {
        ssize_t bytes_read = read(pipe_fd_, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            break;
        }

        size_t samples_count = bytes_read / sizeof(int16_t);
        if (samples_count == 0) continue;

        std::lock_guard<std::mutex> lock(audio_mutex_);
        recorded_samples_.insert(recorded_samples_.end(), buffer, buffer + samples_count);

        for (size_t i = 0; i < samples_count; ++i) {
            float norm = static_cast<float>(buffer[i]) / 32768.0f;
            ring_samples_[ring_head_] = norm;
            ring_head_ = (ring_head_ + 1) % ring_samples_.size();
        }
    }
}

bool AudioRecorder::stop_recording() {
    if (!is_recording_ || recorder_pid_ <= 0) {
        return false;
    }

    is_recording_ = false;

    // Przerwij pw-record
    kill(recorder_pid_, SIGINT);

    // Zamknij deskryptor potoku aby odblokować read()
    if (pipe_fd_ >= 0) {
        close(pipe_fd_);
        pipe_fd_ = -1;
    }

    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    int status = 0;
    for (int i = 0; i < 20; ++i) {
        pid_t res = waitpid(recorder_pid_, &status, WNOHANG);
        if (res == recorder_pid_) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (waitpid(recorder_pid_, &status, WNOHANG) == 0) {
        kill(recorder_pid_, SIGKILL);
        waitpid(recorder_pid_, &status, 0);
    }

    recorder_pid_ = -1;

    // Zapisz zebrane próbki do pliku WAV
    std::vector<int16_t> samples_copy;
    {
        std::lock_guard<std::mutex> lock(audio_mutex_);
        samples_copy = recorded_samples_;
    }

    if (samples_copy.empty()) {
        std::cerr << "Brak próbek audio do zapisu" << std::endl;
        return false;
    }

    return write_wav_file(current_wav_path_, samples_copy, 16000);
}

void AudioRecorder::get_latest_samples(std::vector<float>& out_samples, size_t count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    out_samples.resize(count, 0.0f);
    size_t ring_size = ring_samples_.size();

    for (size_t i = 0; i < count; ++i) {
        // Czytamy od najstarszych do najnowszych próbek z końca bufora kołowego
        size_t idx = (ring_head_ + ring_size - count + i) % ring_size;
        out_samples[i] = ring_samples_[idx];
    }
}

bool AudioRecorder::write_wav_file(const std::string& path, const std::vector<int16_t>& samples, int sample_rate) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Nie można otworzyć pliku do zapisu WAV: " << path << std::endl;
        return false;
    }

    int32_t data_size = static_cast<int32_t>(samples.size() * sizeof(int16_t));
    int32_t chunk_size = 36 + data_size;
    int16_t num_channels = 1;
    int32_t byte_rate = sample_rate * num_channels * sizeof(int16_t);
    int16_t block_align = num_channels * sizeof(int16_t);
    int16_t bits_per_sample = 16;
    int32_t subchunk1_size = 16;
    int16_t audio_format = 1; // PCM

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&chunk_size), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    out.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    out.write(reinterpret_cast<const char*>(&audio_format), 2);
    out.write(reinterpret_cast<const char*>(&num_channels), 2);
    out.write(reinterpret_cast<const char*>(&sample_rate), 4);
    out.write(reinterpret_cast<const char*>(&byte_rate), 4);
    out.write(reinterpret_cast<const char*>(&block_align), 2);
    out.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&data_size), 4);
    out.write(reinterpret_cast<const char*>(samples.data()), data_size);

    return out.good();
}

double AudioRecorder::get_elapsed_seconds() const {
    if (!is_recording_) return 0.0;
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - start_time_;
    return diff.count();
}
