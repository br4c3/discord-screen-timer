#include "command.h"
#include "database.h"
#include "rest.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

static int get_today_range(std::int64_t *day_start, std::int64_t *day_end)
{
    std::time_t now;

    now = std::time(nullptr);

    struct tm local;

#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::cout << "current time: " << now << '\n';

    local.tm_hour = 0;
    local.tm_min  = 0;
    local.tm_sec  = 0;

    std::time_t start;
    start = std::mktime(&local);
    if (start == -1) return -1;

    local.tm_mday += 1;

    std::time_t end;
    end = std::mktime(&local);
    if (end == -1) return -1;

    *day_start = static_cast<std::int64_t>(start);
    *day_end   = static_cast<std::int64_t>(end);

    return 0;
}

static std::string format_duration(std::int64_t seconds)
{
    std::int64_t hours   = seconds / 3600;
    std::int64_t minutes = (seconds % 3600) / 60;

    if (hours > 0) {
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }

    return std::to_string(minutes) + "m";
}

void command_handle_message(const json &data, const char *token)
{
    if (!data.contains("content") || !data.contains("author") || !data.contains("channel_id"))
        return;

    if (data["author"].value("bot", false)) return;

    std::string content = data["content"];

    if (content != "!st") return;

    std::string user_id    = data["author"]["id"];
    std::string channel_id = data["channel_id"];

    std::int64_t day_start;
    std::int64_t day_end;

    if (get_today_range(&day_start, &day_end) < 0) {
        std::cerr << "Failed to get today's time range\n";
        return;
    }

    std::vector<struct activity_summary> result;

    if (database_get_screen_time(user_id, day_start, day_end, result) < 0) {
        std::cerr << "Failed to get screen time\n";
        return;
    }

    std::string  message = "**Today's Screen Time**\n\n";
    std::int64_t total   = 0;

    for (const auto &item : result) {
        message += item.activity + ": ";
        message += format_duration(item.duration);
        message += "\n";

        total += item.duration;
    }

    message += "\nTotal: ";
    message += format_duration(total);

    std::cout << message << '\n';

    /*
    * 디스코드에 메시지 보내기
    */
    if (discord_send_message(token, channel_id, message) < 0) {
        std::cerr << "Failed to respond to !st\n";
    }
}