#include "activity.h"
#include "database.h"

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

using json  = nlohmann::json;
using Clock = std::chrono::steady_clock;

enum activity_type {
    ACTIVITY_PLAYING   = 0,
    ACTIVITY_STREAMING = 1,
    ACTIVITY_LISTENING = 2,
    ACTIVITY_WATCHING  = 3,
    ACTIVITY_CUSTOM    = 4,
    ACTIVITY_COMPETING = 5,
};

struct activity_state {
    std::string                           name;
    Clock::time_point                     start;
    std::chrono::system_clock::time_point started_at;
};

static std::unordered_map<std::string, struct activity_state> users;

static void activity_start(const std::string &user_id, const std::string &name)
{
    struct activity_state state;

    state.name       = name;
    state.start      = Clock::now();
    state.started_at = std::chrono::system_clock::now();

    users[user_id] = state;

    std::cout << "[START] user=" << user_id << " activity=" << name << '\n';
}

static void activity_stop(const std::string &user_id)
{
    auto it = users.find(user_id);

    if (it == users.end()) return;

    auto now      = Clock::now();
    auto ended_at = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.start);

    auto started_timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(it->second.started_at.time_since_epoch());

    auto ended_timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(ended_at.time_since_epoch());

    int ret;

    ret = database_insert_activity(user_id,
                                   it->second.name,
                                   started_timestamp.count(),
                                   ended_timestamp.count(),
                                   elapsed.count());

    if (ret < 0) {
        std::cerr << "Failed to save activity\n";
    } else {
        std::cout << "[SAVE] user=" << user_id << " activity=" << it->second.name
                  << " elapsed=" << elapsed.count() << "s\n";
    }

    users.erase(it);
}

static std::string activity_find_playing(const json &activities)
{
    for (const auto &activity : activities) {
        if (!activity.contains("type") || !activity.contains("name")) continue;

        int type = activity["type"];

        if (type != ACTIVITY_PLAYING) continue;

        return activity["name"];
    }

    return "";
}

void activity_handle_presence(const json &data)
{
    if (!data.contains("user") || !data["user"].contains("id")) return;

    if (!data.contains("activities")) return;

    std::string user_id = data["user"]["id"];
    std::string name    = activity_find_playing(data["activities"]);

    auto it = users.find(user_id);

    /*
     * No trackable activity.
     */
    if (name.empty()) {
        activity_stop(user_id);
        return;
    }

    /*
     * Start tracking a new activity.
     */
    if (it == users.end()) {
        activity_start(user_id, name);
        return;
    }

    /*
     * The same activity is still running.
     */
    if (it->second.name == name) return;

    /*
     * Activity changed.
     */
    activity_stop(user_id);
    activity_start(user_id, name);
}