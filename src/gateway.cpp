#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sys/select.h>

#include "activity.h"
#include "gateway.h"

using json  = nlohmann::json;
using Clock = std::chrono::steady_clock;

enum gateway_intent {
    GATEWAY_INTENT_GUILD_MEMBERS   = 1 << 1,
    GATEWAY_INTENT_GUILD_PRESENCES = 1 << 8,
    GATEWAY_INTENT_GUILD_MESSAGES  = 1 << 9,
    GATEWAY_INTENT_MESSAGE_CONTENT = 1 << 15,
};

enum gateway_opcode {
    GATEWAY_OP_DISPATCH        = 0,
    GATEWAY_OP_HEARTBEAT       = 1,
    GATEWAY_OP_IDENTIFY        = 2,
    GATEWAY_OP_RESUME          = 6,
    GATEWAY_OP_RECONNECT       = 7,
    GATEWAY_OP_INVALID_SESSION = 9,
    GATEWAY_OP_HELLO           = 10,
    GATEWAY_OP_HEARTBEAT_ACK   = 11,
};

struct gateway_state {
    int         sequence;
    int         heartbeat_interval;
    bool        heartbeat_ack;
    std::string session_id;
    std::string resume_gateway_url;
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

static CURLcode gateway_recv(CURL *curl, curl_socket_t sockfd, std::string &message)
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

static CURLcode gateway_send(CURL *curl, const json &payload)
{
    std::string data   = payload.dump();
    size_t      offset = 0;

    while (offset < data.size()) {
        size_t sent = 0;

        CURLcode res =
            curl_ws_send(curl, data.data() + offset, data.size() - offset, &sent, 0, CURLWS_TEXT);

        if (res == CURLE_AGAIN) continue;

        if (res != CURLE_OK) return res;

        offset += sent;
    }

    return CURLE_OK;
}

static CURLcode gateway_send_identify(CURL *curl, const char *token)
{
    const int intents = GATEWAY_INTENT_GUILD_MEMBERS | GATEWAY_INTENT_GUILD_PRESENCES |
                        GATEWAY_INTENT_GUILD_MESSAGES | GATEWAY_INTENT_MESSAGE_CONTENT;

    json payload = {
        {"op", GATEWAY_OP_IDENTIFY},
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

    return gateway_send(curl, payload);
}

static CURLcode gateway_send_heartbeat(CURL *curl, const struct gateway_state *state)
{
    json sequence = nullptr;

    if (state->sequence >= 0) sequence = state->sequence;

    json payload = {
        {"op", GATEWAY_OP_HEARTBEAT},
        {"d", sequence},
    };

    return gateway_send(curl, payload);
}

static int gateway_handle_hello(const json &event, struct gateway_state *state)
{
    if (!event.contains("op") || event["op"] != GATEWAY_OP_HELLO) return -1;

    if (!event.contains("d") || !event["d"].contains("heartbeat_interval")) return -1;

    state->heartbeat_interval = event["d"]["heartbeat_interval"];

    return 0;
}

static void gateway_handle_dispatch(const json &event, struct gateway_state *state)
{
    if (event.contains("s") && !event["s"].is_null()) state->sequence = event["s"];

    if (!event.contains("t") || event["t"].is_null()) return;

    std::string type = event["t"];

    if (type == "READY") {
        const json &data = event["d"];

        if (data.contains("session_id")) state->session_id = data["session_id"];

        if (data.contains("resume_gateway_url"))
            state->resume_gateway_url = data["resume_gateway_url"];
    }
    if (type == "PRESENCE_UPDATE") {
        activity_handle_presence(event["d"]);
    }

    std::cout << "EVENT: " << type << '\n';
}

static long gateway_heartbeat_jitter(int interval)
{
    std::random_device device;
    std::mt19937       generator(device());

    std::uniform_int_distribution<long> distribution(0, interval);

    return distribution(generator);
}

int gateway_connect(const char *token)
{
    CURL       *curl;
    CURLcode    res;
    std::string message;

    struct gateway_state state;

    state.sequence           = -1;
    state.heartbeat_interval = 0;
    state.heartbeat_ack      = true;

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

    curl_socket_t sockfd;

    res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);

    if (res != CURLE_OK) {
        std::cerr << "Failed to get active socket: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "Connected to Discord Gateway\n";

    /*
     * Receive HELLO (OP 10).
     */
    res = gateway_recv(curl, sockfd, message);

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

    if (gateway_handle_hello(hello, &state) < 0) {
        std::cerr << "Invalid HELLO event\n";

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "Heartbeat interval: " << state.heartbeat_interval << " ms\n";

    /*
     * Discord requires the first heartbeat to use a random jitter.
     */
    long jitter = gateway_heartbeat_jitter(state.heartbeat_interval);

    auto next_heartbeat = Clock::now() + std::chrono::milliseconds(jitter);

    /*
     * Send IDENTIFY (OP 2).
     */
    res = gateway_send_identify(curl, token);

    if (res != CURLE_OK) {
        std::cerr << "Failed to send IDENTIFY: " << curl_easy_strerror(res) << '\n';

        curl_easy_cleanup(curl);
        return 1;
    }

    std::cout << "IDENTIFY sent\n";

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
         * Gateway data is available.
         */
        if (ret > 0) {
            message.clear();

            res = gateway_recv(curl, sockfd, message);

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

            if (!event.contains("op")) continue;

            int opcode = event["op"];

            switch (opcode) {
            case GATEWAY_OP_DISPATCH:
                gateway_handle_dispatch(event, &state);
                break;

            case GATEWAY_OP_HEARTBEAT:
                res = gateway_send_heartbeat(curl, &state);

                if (res != CURLE_OK) {
                    std::cerr << "Failed to respond to heartbeat request\n";
                    goto cleanup;
                }

                state.heartbeat_ack = false;

                std::cout << "HEARTBEAT sent (requested)\n";
                break;

            case GATEWAY_OP_RECONNECT:
                std::cout << "RECONNECT requested\n";
                goto cleanup;

            case GATEWAY_OP_INVALID_SESSION:
                std::cout << "INVALID SESSION\n";
                goto cleanup;

            case GATEWAY_OP_HEARTBEAT_ACK:
                state.heartbeat_ack = true;

                std::cout << "HEARTBEAT ACK\n";
                break;

            default:
                break;
            }
        }

        /*
         * Send the scheduled heartbeat.
         */
        now = Clock::now();

        if (now >= next_heartbeat) {
            if (!state.heartbeat_ack) {
                std::cerr << "Heartbeat ACK timeout\n";
                break;
            }

            res = gateway_send_heartbeat(curl, &state);

            if (res != CURLE_OK) {
                std::cerr << "Failed to send HEARTBEAT: " << curl_easy_strerror(res) << '\n';
                break;
            }

            state.heartbeat_ack = false;

            std::cout << "HEARTBEAT sent (sequence: ";

            if (state.sequence >= 0)
                std::cout << state.sequence;
            else
                std::cout << "null";

            std::cout << ")\n";

            next_heartbeat = now + std::chrono::milliseconds(state.heartbeat_interval);
        }
    }

cleanup:
    curl_easy_cleanup(curl);

    return 0;
}