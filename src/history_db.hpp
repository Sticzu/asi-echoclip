#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sqlite3.h>

struct HistoryEntry {
    int64_t id{0};
    std::string timestamp;
    double duration_sec{0.0};
    std::string text;
};

class HistoryDb {
public:
    HistoryDb();
    ~HistoryDb();

    bool init();
    bool add_entry(const std::string& text, double duration_sec);
    std::vector<HistoryEntry> get_all_entries(const std::string& search_filter = "");
    bool delete_entry(int64_t id);
    bool clear_all();

private:
    sqlite3* db_{nullptr};
    std::string db_path_;
    std::string get_default_db_path();
};
