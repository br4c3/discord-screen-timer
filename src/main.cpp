#include <cstdlib>
#include <iostream>
#include <string>

#include <curl/curl.h>

#include <sys/select.h>

enum GatewayIntent {
    GUILD_PRESENCES = 1 << 8,
    GUILD_MESSAGES  = 1 << 9,
    MESSAGE_CONTENT = 1 << 15,
};

static int wait_socket(curl_socket_t sockfd, long timeout_ms)
{
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    struct timeval timeout;

    timeout.tv_sec  = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
}

static CURLcode recv_message(CURL *curl, curl_socket_t sockfd, std::string &message)
{
    char                        buffer[4096];
    size_t                      received;
    const struct curl_ws_frame *meta;
    CURLcode                    res;

    while (true) {
        received = 0;
        meta     = nullptr;

        res = curl_ws_recv(curl, buffer, sizeof(buffer), &received, &meta);

        if (res == CURLE_AGAIN) {
            int ret = wait_socket(sockfd, 1000);

            if (ret < 0) return CURLE_RECV_ERROR;

            continue;
        }

        if (res != CURLE_OK) return res;

        if (!meta) return CURLE_RECV_ERROR;

        message.append(buffer, received);

        if (meta->bytesleft == 0) break;
    }

    return CURLE_OK;
}

static CURLcode send_identify(CURL *curl, const char *token)
{
    const int intents = GUILD_PRESENCES | GUILD_MESSAGES | MESSAGE_CONTENT;

    std::string payload = "{"
                          "\"op\":2,"
                          "\"d\":{"
                          "\"token\":\"" +
                          std::string(token) +
                          "\","
                          "\"intents\":" +
                          std::to_string(intents) +
                          ","
                          "\"properties\":{"
                          "\"os\":\"linux\","
                          "\"browser\":\"discord-screen-timer\","
                          "\"device\":\"discord-screen-timer\""
                          "}"
                          "}"
                          "}";

    size_t sent = 0;

    return curl_ws_send(curl, payload.data(), payload.size(), &sent, 0, CURLWS_TEXT);
}

/**
 * @brief Connects to the Discord Gateway and receives the HELLO event.
 *
 * @return Zero on success, non-zero on failure.
 */
static int gateway_connect(const char *token)
{
    CURL       *curl;
    CURLcode    res;
    std::string message;

    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize curl\n";
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "wss://gateway.discord.gg/?v=10&encoding=json");

    /*
     * Perform the WebSocket handshake and return control to the
     * application after the connection has been established.
     */
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Gateway connection failed: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "Connected to Discord Gateway\n";

    curl_socket_t sockfd;

    res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);

    if (res != CURLE_OK) {
        std::cerr << "Failed to get active socket: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    /*
     * Receive HELLO (OP 10).
     */
    res = recv_message(curl, sockfd, message);

    if (res != CURLE_OK) {
        std::cerr << "Failed to receive HELLO: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "HELLO: " << message << '\n';

    /*
     * Send IDENTIFY (OP 2).
     */
    res = send_identify(curl, token);

    if (res != CURLE_OK) {
        std::cerr << "Failed to send IDENTIFY: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "IDENTIFY sent\n";

    /*
     * Receive Gateway events.
     */
    while (true) {
        message.clear();

        res = recv_message(curl, sockfd, message);

        if (res != CURLE_OK) {
            std::cerr << "Gateway receive failed: " << curl_easy_strerror(res) << '\n';
            break;
        }

        std::cout << "EVENT: " << message << '\n';
    }

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

    int ret = gateway_connect(token);

    curl_global_cleanup();

    return ret;
}