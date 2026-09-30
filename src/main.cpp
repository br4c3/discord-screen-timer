#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sys/select.h>

using json  = nlohmann::json;
using Clock = std::chrono::steady_clock;

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

static CURLcode send_json(CURL *curl, const json &payload)
{
    std::string data = payload.dump();
    size_t      sent = 0;

    return curl_ws_send(curl, data.data(), data.size(), &sent, 0, CURLWS_TEXT);
}

static CURLcode send_identify(CURL *curl, const char *token)
{
    const int intents = GUILD_PRESENCES | GUILD_MESSAGES | MESSAGE_CONTENT;

    json payload = {
        {"op", 2},
        {"d",
         {
             {"token", token},
             {"intents", intents},
             {"properties",
              {
                  {"os", "linux"},
                  {"browser", "discord-screen-timer"},
                  {"device", "discord-screen-timer"},
              }},
         }},
    };

    return send_json(curl, payload);
}

static CURLcode send_heartbeat(CURL *curl, int sequence)
{
    json payload = {
        {"op", 1},
        {"d", sequence},
    };

    return send_json(curl, payload);
}

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

    json hello;

    try {
        hello = json::parse(message);
    } catch (const json::parse_error &e) {
        std::cerr << "Failed to parse HELLO: " << e.what() << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    if (hello["op"] != 10) {
        std::cerr << "Expected HELLO event\n";

        curl_easy_cleanup(curl);
        return 1;
    }

    int heartbeat_interval = hello["d"]["heartbeat_interval"];

    std::cout << "Heartbeat interval: " << heartbeat_interval << " ms\n";

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

    int sequence = 0;

    auto next_heartbeat = Clock::now() + std::chrono::milliseconds(heartbeat_interval);

    /*
     * Gateway event loop.
     */
    while (true) {
        auto now = Clock::now();

        auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(next_heartbeat - now);

        long timeout_ms = remaining.count();

        if (timeout_ms < 0) timeout_ms = 0;

        int ret = wait_socket(sockfd, timeout_ms);

        if (ret < 0) {
            std::cerr << "select() failed\n";
            break;
        }

        /*
         * Receive Gateway event.
         */
        if (ret > 0) {
            message.clear();

            res = recv_message(curl, sockfd, message);

            if (res != CURLE_OK) {
                std::cerr << "Gateway receive failed: " << curl_easy_strerror(res) << '\n';
                break;
            }

            json event;

            try {
                event = json::parse(message);
            } catch (const json::parse_error &e) {
                std::cerr << "Failed to parse Gateway event: " << e.what() << '\n';
                continue;
            }

            if (event.contains("s") && !event["s"].is_null()) sequence = event["s"];

            int opcode = event["op"];

            if (opcode == 11) {
                std::cout << "HEARTBEAT ACK\n";
            } else if (opcode == 0) {
                std::cout << "EVENT: " << event["t"] << '\n';
            }
        }

        /*
         * Send HEARTBEAT (OP 1).
         */
        now = Clock::now();

        if (now >= next_heartbeat) {
            res = send_heartbeat(curl, sequence);

            if (res != CURLE_OK) {
                std::cerr << "Failed to send HEARTBEAT: " << curl_easy_strerror(res) << '\n';
                break;
            }

            std::cout << "HEARTBEAT sent"
                      << " (sequence: " << sequence << ")\n";

            next_heartbeat = now + std::chrono::milliseconds(heartbeat_interval);
        }
    }

    curl_easy_cleanup(curl);

    return 0;
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