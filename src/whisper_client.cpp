#include "whisper_client.hpp"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>

WhisperClient::WhisperClient(std::string base_url) 
    : base_url_(std::move(base_url)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* env_url = std::getenv("WHISPER_SERVER_URL");
    if (env_url && *env_url) {
        candidate_urls_.push_back(env_url);
    }
    if (!base_url_.empty()) {
        candidate_urls_.push_back(base_url_);
    }
    candidate_urls_.push_back("http://127.0.0.1:8999");
    candidate_urls_.push_back("http://localhost:8999");
    candidate_urls_.push_back("http://host.docker.internal:8999");
    candidate_urls_.push_back("http://sticzupc:8999");
    candidate_urls_.push_back("http://100.72.137.46:8999");

    if (base_url_.empty()) {
        base_url_ = candidate_urls_.front();
    }
}

WhisperClient::~WhisperClient() {
    curl_global_cleanup();
}

size_t WhisperClient::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

bool WhisperClient::probe_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && http_code >= 200 && http_code < 400);
}

bool WhisperClient::check_server_online() {
    // 1. Sprawdź najpierw aktualny URL
    if (probe_url(base_url_)) {
        return true;
    }

    // 2. Jeśli nie odpowiada, przeskanuj listę kandydatów (np. Tailscale sticzupc)
    for (const auto& url : candidate_urls_) {
        if (url == base_url_) continue;
        if (probe_url(url)) {
            base_url_ = url;
            return true;
        }
    }

    return false;
}

std::string WhisperClient::get_active_host_display() const {
    if (base_url_.find("100.72.137.46") != std::string::npos || 
        base_url_.find("sticzupc") != std::string::npos) {
        return base_url_ + " [Tailscale sticzupc]";
    }
    if (base_url_.find("127.0.0.1") != std::string::npos || 
        base_url_.find("localhost") != std::string::npos) {
        return base_url_ + " [Lokalny]";
    }
    return base_url_;
}

std::pair<bool, std::string> WhisperClient::transcribe(const std::string& wav_file_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return {false, "Błąd inicjalizacji biblioteki curl."};
    }

    std::string endpoint = base_url_ + "/inference";
    std::string read_buffer;

    curl_mime* form = curl_mime_init(curl);

    curl_mimepart* part_file = curl_mime_addpart(form);
    curl_mime_name(part_file, "file");
    CURLcode f_res = curl_mime_filedata(part_file, wav_file_path.c_str());
    if (f_res != CURLE_OK) {
        curl_mime_free(form);
        curl_easy_cleanup(curl);
        return {false, "Nie można odczytać pliku nagrania WAV: " + wav_file_path};
    }

    curl_mimepart* part_fmt = curl_mime_addpart(form);
    curl_mime_name(part_fmt, "response_format");
    curl_mime_data(part_fmt, "json", CURL_ZERO_TERMINATED);

    curl_mimepart* part_temp = curl_mime_addpart(form);
    curl_mime_name(part_temp, "temperature");
    curl_mime_data(part_temp, "0.0", CURL_ZERO_TERMINATED);

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_mime_free(form);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return {false, std::string("Błąd połączenia z Whisper (" + base_url_ + "): ") + curl_easy_strerror(res)};
    }

    if (http_code != 200) {
        return {false, "Serwer Whisper zwrócił kod HTTP " + std::to_string(http_code) + ": " + read_buffer};
    }

    try {
        auto json_data = nlohmann::json::parse(read_buffer);
        if (json_data.contains("error")) {
            return {false, "Błąd Whisper: " + json_data["error"].get<std::string>()};
        }

        std::string text = json_data.value("text", "");

        size_t start = text.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return {true, ""};
        }
        size_t end = text.find_last_not_of(" \t\n\r");
        text = text.substr(start, end - start + 1);

        return {true, text};
    } catch (const std::exception& e) {
        return {false, std::string("Błąd parsowania odpowiedzi JSON: ") + e.what()};
    }
}
