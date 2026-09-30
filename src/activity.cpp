#include "activity.h"

#include <iostream>
#include <string>

using json = nlohmann::json;

void activity_handle_presence(const json &data)
{
    if (!data.contains("user")) return;

    if (!data["user"].contains("id")) return;

    std::string user_id = data["user"]["id"];

    if (!data.contains("activities")) return;

    const json &activities = data["activities"];

    if (activities.empty()) {
        std::cout << "[PRESENCE] user=" << user_id << " activity=None\n";
        return;
    }

    for (const auto &activity : activities) {
        if (!activity.contains("name")) continue;

        std::string name = activity["name"];

        std::cout << "[PRESENCE] user=" << user_id << " activity=" << name << '\n';
    }
}