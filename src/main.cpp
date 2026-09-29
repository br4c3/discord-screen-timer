#include <cstdlib>
#include <iostream>
#include <string>

#include <curl/curl.h>

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;

    static_cast<std::string *>(userp)->append(static_cast<char *>(contents), total);

    return total;
}

int main(void)
{
    const char *token = std::getenv("DISCORD_TOKEN");

    if (!token) {
        std::cerr << "DISCORD_TOKEN is not set\n";
        return 1;
    }

    CURL       *curl;
    CURLcode    res;
    std::string response;

    curl = curl_easy_init();

    if (!curl) return 1;

    curl_easy_setopt(curl, CURLOPT_URL, "https://discord.com/api/v10/gateway");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << curl_easy_strerror(res) << '\n';
        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << response << '\n';

    curl_easy_cleanup(curl);

    return 0;
}