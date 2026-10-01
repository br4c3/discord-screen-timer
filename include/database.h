#ifndef DATABASE_H
#define DATABASE_H

#include <cstdint>
#include <vector>
#include <string>

int database_init(const char *path);

int database_insert_activity(const std::string &user_id,
                             const std::string &activity,
                             std::int64_t       started_at,
                             std::int64_t       ended_at,
                             std::int64_t       duration);

struct activity_summary {
    std::string  activity;
    std::int64_t duration;
};

int database_get_screen_time(const std::string                    &user_id,
                             std::vector<struct activity_summary> &result);

void database_close(void);

#endif