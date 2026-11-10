#pragma once

#include <string>
#include <vector>
#include <utility>

class WhisperClient {
public:
    WhisperClient(std::string base_url = "");
    ~WhisperClient();

    bool check_server_online();
    std::pair<bool, std::string> transcribe(const std::string& wav_file_path);

    void set_base_url(const std::string& url) { base_url_ = url; }
    std::string get_base_url() const { return base_url_; }
    std::string get_active_host_display() const;

private:
    std::string base_url_;
    std::vector<std::string> candidate_urls_;
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    bool probe_url(const std::string& url);
};
