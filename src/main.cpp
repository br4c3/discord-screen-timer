#include <cstdlib>
#include <iostream>
#include <string>

#include <curl/curl.h>

/**
 * @brief Handles data received from a libcurl request.
 *
 * Appends the received data to the string specified by the user pointer.
 *
 * @param contents Pointer to the received data.
 * @param size Size of each data element in bytes.
 * @param nmemb Number of received data elements.
 * @param userp Pointer to the destination std::string.
 *
 * @return Number of bytes successfully processed.
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;

    std::string *response = static_cast<std::string *>(userp);
    char        *data     = static_cast<char *>(contents);

    response->append(data, total);

    return total;
}

int main(void)
{
    // Check discord token
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