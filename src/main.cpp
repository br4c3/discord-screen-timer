#include <cstdlib>
#include <iostream>
#include <string>

#include <curl/curl.h>


static CURLcode recv_message(CURL *curl, std::string& message) {
    char buffer[4096];
    size_t received;
    const struct curl_ws_frame *meta;
    CURLcode res;

    do {
        res = curl_ws_recv(curl, buffer, sizeof(buffer), &received, &meta);

        if (res == CURLE_AGAIN)
            continue;
        
        if (res != CURLE_OK)
            return res;
        
            message.append(buffer, received);
    } while (meta->bytesleft > 0);

    return CURLE_OK;
}


/**
 * @brief Connects to the Discord Gateway and receives the HELLO event.
 *
 * @return Zero on success, non-zero on failure.
 */
static int gateway_connect(void)
{
    CURL       *curl;
    CURLcode    res;
    std::string message;

    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize curl\n";
        return 1;
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "wss://gateway.discord.gg/?v=10&encoding=json"
    );

    /*
     * Perform the WebSocket handshake and return control to the
     * application after the connection has been established.
     */
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Gateway connection failed: "
                  << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "Connected to Discord Gateway\n";

    res = recv_message(curl, message);

    if (res != CURLE_OK) {
        std::cerr << "Failed to receive Gateway message: "
                  << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << message << '\n';

    curl_easy_cleanup(curl);

    return 0;
}

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
    const char *token = std::getenv("DISCORD_TOKEN");

    if (!token) {
        std::cerr << "DISCORD_TOKEN is not set\n";
        return 1;
    }

    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (res != CURLE_OK) {
        std::cerr << "Failed to initialize libcurl\n";
        return 1;
    }

    int ret = gateway_connect();

    curl_global_cleanup();

    return ret;
}