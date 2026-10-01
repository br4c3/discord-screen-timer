#ifndef COMMAND_H
#define COMMAND_H

#include <nlohmann/json.hpp>

void command_handle_message(const nlohmann::json &data, const char *token);

#endif