#include "rest.h"

#include <iostream>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int discord_send_message(const char        *token,
                         const std::string &channel_id,
                         const std::string &content)
{
    CURL              *curl;
    CURLcode           res;
    struct curl_slist *headers;
    long               status;

    curl    = curl_easy_init();
    headers = nullptr;
    status  = 0;

    if (!curl) return -1;

    std::string url = "https://discord.com/api/v10/channels/" + channel_id + "/messages";

    std::string authorization = "Authorization: Bot " + std::string(token);

    json payload = {
        {"content", content},
    };

    std::string body = payload.dump();

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (!headers) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Failed to send message: " << curl_easy_strerror(res) << '\n';
        goto cleanup;
    }

    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (res != CURLE_OK) {
        std::cerr << "Failed to get HTTP status\n";
        goto cleanup;
    }

    if (status < 200 || status >= 300) {
        std::cerr << "Discord API returned HTTP " << status << '\n';

        res = CURLE_HTTP_RETURNED_ERROR;
        goto cleanup;
    }

cleanup:
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return res == CURLE_OK ? 0 : -1;
}