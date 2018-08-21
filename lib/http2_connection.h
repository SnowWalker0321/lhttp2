#ifndef _HTTP2_CONNECTION_H
#define _HTTP2_CONNECTION_H

#include <vector>
#include <stdint.h>

#include "http2_stream.h"
#include "http2_frame.h"

// The client connection preface starts with a sequence of 24 octets,
// "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define PREFACE "\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a"
#define PREFACE_LEN 24

typedef enum _HTTP2_ENDPOINT_TYPE {
    HTTP2_ENDPOINT_CLIENT,
    HTTP2_ENDPOINT_SERVER,
} HTTP2_ENDPOINT_TYPE;

class http2_connection {
public:
    http2_connection();
    http2_connection(HTTP2_ENDPOINT_TYPE type);

    uint32_t allocate_client_stream();
    uint32_t allocate_server_stream();

    void send_frame(uint32_t id, http2_frame* frame);
    void recv_frame();

    uint32_t get_last_client_stream_id();
    uint32_t get_last_server_stream_id();
    HTTP2_STREAM_STATUS get_stream_status(int id);

private:
    int fd;
    HTTP2_ENDPOINT_TYPE endpoint_type;
    std::vector<http2_stream> stream_vector;
    uint32_t window_size = 65535;
    http2_settings settings;
};

void http2_send_preface(int fd);
bool http2_recv_preface(int fd);

#endif