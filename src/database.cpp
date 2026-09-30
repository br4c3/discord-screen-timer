#include "database.h"

#include <iostream>

#include <sqlite3.h>

static sqlite3 *db = nullptr;

static const char *CREATE_ACTIVITY_TABLE = "CREATE TABLE IF NOT EXISTS activity_log ("
                                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                           "user_id TEXT NOT NULL,"
                                           "activity TEXT NOT NULL,"
                                           "started_at INTEGER NOT NULL,"
                                           "ended_at INTEGER NOT NULL,"
                                           "duration INTEGER NOT NULL"
                                           ");";

int database_init(const char *path)
{
    int ret;

    ret = sqlite3_open(path, &db);

    if (ret != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << '\n';

        return -1;
    }

    ret = sqlite3_exec(db, CREATE_ACTIVITY_TABLE, nullptr, nullptr, nullptr);

    if (ret != SQLITE_OK) {
        std::cerr << "Failed to create activity table: " << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        db = nullptr;

        return -1;
    }

    return 0;
}

int database_insert_activity(const std::string &user_id,
                             const std::string &activity,
                             std::int64_t       started_at,
                             std::int64_t       ended_at,
                             std::int64_t       duration)
{
    static const char *sql = "INSERT INTO activity_log "
                             "(user_id, activity, started_at, ended_at, duration) "
                             "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int           ret;

    if (!db) return -1;

    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (ret != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, activity.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, started_at);
    sqlite3_bind_int64(stmt, 4, ended_at);
    sqlite3_bind_int64(stmt, 5, duration);

    ret = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE) {
        std::cerr << "Failed to insert activity: " << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    return 0;
}

void database_close(void)
{
    if (!db) return;

    sqlite3_close(db);
    db = nullptr;
}