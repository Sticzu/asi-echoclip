#include "history_db.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>

HistoryDb::HistoryDb() {
    db_path_ = get_default_db_path();
}

HistoryDb::~HistoryDb() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

std::string HistoryDb::get_default_db_path() {
    const char* xdg_data = std::getenv("XDG_DATA_HOME");
    std::filesystem::path dir;
    if (xdg_data && *xdg_data) {
        dir = std::filesystem::path(xdg_data) / "asi-echoclip";
    } else {
        const char* home = std::getenv("HOME");
        if (home && *home) {
            dir = std::filesystem::path(home) / ".local" / "share" / "asi-echoclip";
        } else {
            dir = "/tmp/asi-echoclip";
        }
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return (dir / "history.db").string();
}

bool HistoryDb::init() {
    if (db_) return true;

    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Nie udało się otworzyć bazy historii SQLite: " 
                  << (db_ ? sqlite3_errmsg(db_) : "unknown") << std::endl;
        return false;
    }

    const char* schema = 
        "CREATE TABLE IF NOT EXISTS transcriptions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT (datetime('now', 'localtime')),"
        "  duration REAL,"
        "  text TEXT NOT NULL"
        ");";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db_, schema, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "Błąd inicjalizacji schematu SQLite: " 
                  << (err_msg ? err_msg : "") << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool HistoryDb::add_entry(const std::string& text, double duration_sec) {
    if (!init()) return false;

    const char* query = "INSERT INTO transcriptions (duration, text) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_double(stmt, 1, duration_sec);
    sqlite3_bind_text(stmt, 2, text.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<HistoryEntry> HistoryDb::get_all_entries(const std::string& search_filter) {
    std::vector<HistoryEntry> entries;
    if (!init()) return entries;

    std::string query = "SELECT id, timestamp, duration, text FROM transcriptions ";
    if (!search_filter.empty()) {
        query += "WHERE text LIKE ? ";
    }
    query += "ORDER BY id DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return entries;
    }

    if (!search_filter.empty()) {
        std::string param = "%" + search_filter + "%";
        sqlite3_bind_text(stmt, 1, param.c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry entry;
        entry.id = sqlite3_column_int64(stmt, 0);
        const unsigned char* ts = sqlite3_column_text(stmt, 1);
        entry.timestamp = ts ? reinterpret_cast<const char*>(ts) : "";
        entry.duration_sec = sqlite3_column_double(stmt, 2);
        const unsigned char* txt = sqlite3_column_text(stmt, 3);
        entry.text = txt ? reinterpret_cast<const char*>(txt) : "";
        entries.push_back(std::move(entry));
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool HistoryDb::delete_entry(int64_t id) {
    if (!init()) return false;

    const char* query = "DELETE FROM transcriptions WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool HistoryDb::clear_all() {
    if (!init()) return false;

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM transcriptions;", nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    return true;
}
