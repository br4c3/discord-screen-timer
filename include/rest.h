#ifndef REST_H
#define REST_H

#include <string>

int discord_send_message(const char *token,
                         const std::string &channel_id,
                         const std::string &content);

#endif