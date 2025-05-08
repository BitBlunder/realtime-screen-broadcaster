#ifndef WS_CLIENT_HPP
#define WS_CLIENT_HPP

#define WS_URL_LEN 128

#include <cstdint>

struct WsClientContext;

struct WsClientConfig
{
	char url [WS_URL_LEN];
};

WsClientContext*
ws_init(const WsClientConfig& ws_config);

void
ws_request_stop(WsClientContext* self);

int
ws_stop_requested(WsClientContext* self);

int
ws_send_frame(WsClientContext* self, const uint8_t* data, size_t len);

void
ws_free(WsClientContext* self);

#endif