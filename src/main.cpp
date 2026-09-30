#include <cstdlib>
#include <iostream>

#include <curl/curl.h>

#include "database.h"
#include "gateway.h"

int main(void)
{
    const char *token = std::getenv("DISCORD_TOKEN");

    if (!token) {
        std::cerr << "DISCORD_TOKEN is not set\n";
        return 1;
    }

    CURLcode res;

    res = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (res != CURLE_OK) {
        std::cerr << "Failed to initialize libcurl\n";
        return 1;
    }

    if (database_init("screen-time.db") < 0) {
        curl_global_cleanup();
        return 1;
    }

    int ret;

    ret = gateway_connect(token);

    database_close();
    curl_global_cleanup();

    return ret;
}