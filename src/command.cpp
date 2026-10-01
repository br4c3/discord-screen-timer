#include "command.h"
#include "database.h"

#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

static std::string format_duration(std::int64_t seconds)
{
    std::int64_t hours   = seconds / 3600;
    std::int64_t minutes = (seconds % 3600) / 60;

    if (hours > 0) {
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }

    return std::to_string(minutes) + "m";
}

void command_handle_message(const json &data)
{
    if (!data.contains("content") || !data.contains("author") || !data.contains("channel_id"))
        return;

    if (data["author"].value("bot", false)) return;

    std::string content = data["content"];

    if (content != "!st") return;

    std::string user_id    = data["author"]["id"];
    std::string channel_id = data["channel_id"];

    std::vector<struct activity_summary> result;

    if (database_get_screen_time(user_id, result) < 0) {
        std::cerr << "Failed to get screen time\n";
        return;
    }

    std::string  message = "**Screen Time**\n\n";
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
     * Next:
     * discord_send_message(channel_id, message);
     */
}