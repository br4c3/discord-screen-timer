#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <nlohmann/json.hpp>

void activity_handle_presence(const nlohmann::json &data);

#endif