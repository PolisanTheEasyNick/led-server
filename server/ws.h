#ifdef libwebsockets_FOUND
#ifndef WS_H
#define WS_H

#include <libwebsockets.h>

int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

typedef struct
{
    char message[64];
    int has_message;
} ws_client_data;

static struct lws_protocols protocols[] = {
    {
        "piled",
        callback_websocket,
        .per_session_data_size = sizeof(ws_client_data),
        0,
    },
    {NULL, NULL, 0, 0} // terminator
};

void ws_server_init(uint8_t pi_);
void ws_broadcast_color(struct Color color);

#endif
#endif
