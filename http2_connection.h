#ifndef _HTTP2_CONNECTION_H
#define _HTTP2_CONNECTION_H

#include <vector>
#include <stdint.h>

#include "http2_stream.h"
#include "http2_frame.h"

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

#endif