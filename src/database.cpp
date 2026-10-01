#include "database.h"

#include <iostream>

#include <sqlite3.h>

static sqlite3 *db = nullptr;

static const char *CREATE_ACTIVITY_TABLE = "CREATE TABLE IF NOT EXISTS activity_log ("
                                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                           "user_id TEXT NOT NULL,"
                                           "activity TEXT NOT NULL,"
                                           "date TEXT NOT NULL,"
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

int database_get_screen_time(const std::string                    &user_id,
                             std::int64_t                          day_start,
                             std::int64_t                          day_end,
                             std::vector<struct activity_summary> &result)
{
    /*
     * 시작: 2026-09-30 23:50
     * 종료: 2026-10-01 00:20
     * 이런 경우 예외처리를 위해서, 겹치는 시간을 계산
     * started_at < day_end
     * AND ended_at > day_start
     *
     * 오늘 해당하는 부분만 계산
     * MIN(ended_at, day_end)
     * -
     * MAX(started_at, day_start)
     */
    static const char *sql = "SELECT activity, "
                             "SUM(MIN(ended_at, ?) - MAX(started_at, ?)) "
                             "FROM activity_log "
                             "WHERE user_id = ? "
                             "AND started_at < ? "
                             "AND ended_at > ? "
                             "GROUP BY activity "
                             "ORDER BY SUM(MIN(ended_at, ?) - MAX(started_at, ?)) DESC;";

    sqlite3_stmt *stmt;
    int           ret;

    if (!db) return -1;

    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (ret != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, day_start);
    sqlite3_bind_text(stmt, 3, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, day_end);
    sqlite3_bind_int64(stmt, 5, day_start);
    sqlite3_bind_int64(stmt, 6, day_end);
    sqlite3_bind_int64(stmt, 7, day_start);

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct activity_summary summary;

        summary.activity = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        summary.duration = sqlite3_column_int64(stmt, 1);

        result.push_back(summary);
    }

    sqlite3_finalize(stmt);

    return ret == SQLITE_DONE ? 0 : -1;
}

void database_close(void)
{
    if (!db) return;

    sqlite3_close(db);
    db = nullptr;
}