#include <pigpiod_if2.h>
#ifdef libwebsockets_FOUND
#include "../globals/globals.h"
#include "../rgb/gpio.h"
#include "../utils/utils.h"
#include "server.h"
#include "ws.h"
#include <pthread.h>

#define MAX_WS_CLIENTS 10
static struct lws *ws_clients[MAX_WS_CLIENTS];
static int ws_clients_count = 0;
static pthread_mutex_t ws_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int
callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    if (is_suspended)
    {
        return 0;
    }

    switch (reason)
    {
        case LWS_CALLBACK_RECEIVE:
        {
            // expecting the input in the format: "RED,GREEN,BLUE,DURATION"
            char *msg = (char *)in;
            char *token;
            int red, green, blue, duration;

            token = strtok(msg, ",");

            if (token)
            {
                red = atoi(token);
            }

            token = strtok(NULL, ",");

            if (token)
            {
                green = atoi(token);
            }

            token = strtok(NULL, ",");

            if (token)
            {
                blue = atoi(token);
            }

            token = strtok(NULL, ",");

            if (token)
            {
                duration = atoi(token);
            }

            set_color_duration(pi, (struct Color) {red, green, blue}, duration);
            break;
        }

        case LWS_CALLBACK_ESTABLISHED:
        {
            pthread_mutex_lock(&ws_clients_mutex);

            if (ws_clients_count < MAX_WS_CLIENTS)
            {
                ws_clients[ws_clients_count++] = wsi;
                logger_debug(WS, "WebSocket client connected, total: %d", ws_clients_count);

                // Fetch current color from GPIO
                struct Color current = {
                    get_PWM_dutycycle(pi, RED_PIN),
                    get_PWM_dutycycle(pi, GREEN_PIN),
                    get_PWM_dutycycle(pi, BLUE_PIN)
                };

                ws_client_data *client_data = (ws_client_data *)user;

                snprintf(client_data->message,
                         sizeof(client_data->message),
                         "%d,%d,%d",
                         current.RED,
                         current.GREEN,
                         current.BLUE);
                client_data->has_message = 1;
                lws_callback_on_writable(wsi);
            }
            else
            {
                logger_debug(WS, "Too many WebSocket clients");
            }

            pthread_mutex_unlock(&ws_clients_mutex);
            break;
        }

        case LWS_CALLBACK_CLOSED:
        {
            pthread_mutex_lock(&ws_clients_mutex);

            for (int i = 0; i < ws_clients_count; i++)
            {
                if (ws_clients[i] == wsi)
                {
                    // shift clients left
                    for (int j = i; j < ws_clients_count - 1; j++)
                    {
                        ws_clients[j] = ws_clients[j + 1];
                    }

                    ws_clients_count--;
                    logger_debug(WS, "WebSocket client disconnected, remaining: %d", ws_clients_count);
                    break;
                }
            }

            pthread_mutex_unlock(&ws_clients_mutex);
            break;
        }

        case LWS_CALLBACK_SERVER_WRITEABLE:
        {
            ws_client_data *client_data = (ws_client_data *)user;

            if (client_data->has_message)
            {
                unsigned char buf[LWS_PRE + 64];
                unsigned char *p = &buf[LWS_PRE];
                size_t msg_len = strlen(client_data->message);

                memcpy(p, client_data->message, msg_len);
                int written = lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);

                if (written < msg_len)
                {
                    logger_debug(WS, "Failed to send initial color to WS client");
                }
                else
                {
                    logger_debug(WS, "Sent initial color to WS client: %s", client_data->message);
                }

                client_data->has_message = 0;  // Reset the flag
            }

            break;
        }

        default:
            break;
    }

    return 0;
}

void *
event_loop(void *arg)
{
    struct lws_context *context = (struct lws_context *)arg;

    while (!stop_server)
    {
        lws_service(context, 0);
    }

    return NULL;
}

void
ws_server_init(uint8_t pi_)
{
    pi = pi_;
    struct lws_context_creation_info info;
    struct lws_context *context;

    memset(&info, 0, sizeof(info));
    info.port = 3385;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    lws_set_log_level(0, NULL);

    context = lws_create_context(&info);

    if (!context)
    {
        logger_debug(WS, "lws create context failed\n");

        return;
    }

    pthread_t thread;

    if (pthread_create(&thread, NULL, event_loop, context) != 0)
    {
        logger_debug(WS, "Failed to create event loop thread\n");
        lws_context_destroy(context);

        return;
    }

    logger(WS, "Started WS server on port 3385 successfully!");

    pthread_detach(thread);
}

void
ws_broadcast_color(struct Color color)
{
    pthread_mutex_lock(&ws_clients_mutex);

    char message[64];
    int len = snprintf(message, sizeof(message), "%d,%d,%d", color.RED, color.GREEN, color.BLUE);

    for (int i = 0; i < ws_clients_count; i++)
    {
        if (lws_callback_on_writable(ws_clients[i]))
        {
            unsigned char buf[LWS_PRE + 64];
            unsigned char *p = &buf[LWS_PRE];
            memcpy(p, message, len);

            int written = lws_write(ws_clients[i], p, len, LWS_WRITE_TEXT);

            if (written < len)
            {
                logger_debug(WS, "Failed to write to WS client %d", i);
            }
        }
        else
        {
            logger_debug(WS, "WS client %d not writeable", i);
        }
    }

    pthread_mutex_unlock(&ws_clients_mutex);
}

#endif
