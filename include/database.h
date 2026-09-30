#ifndef DATABASE_H
#define DATABASE_H

#include <cstdint>
#include <string>

int database_init(const char *path);

int database_insert_activity(const std::string &user_id,
                             const std::string &activity,
                             std::int64_t       started_at,
                             std::int64_t       ended_at,
                             std::int64_t       duration);

void database_close(void);

#endif