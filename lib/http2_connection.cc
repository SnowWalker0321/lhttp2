#include "http2_connection.h"

http2_connection::http2_connection() {
}

http2_connection::http2_connection(HTTP2_ENDPOINT_TYPE type) : endpoint_type(type) {
    if(type == HTTP2_ENDPOINT_CLIENT) {
    } else if(type == HTTP2_ENDPOINT_SERVER) {
    }
}

uint32_t http2_connection::allocate_client_stream() {
    for(int i = 1; i < stream_vector.size(); i++) {
        if(stream_vector[i].get_status() == HTTP2_STREAM_IDLE) {
            stream_vector[i].set_status(HTTP2_STREAM_CLOSED);
        }
    }
    return 0;
}

void http2_connection::send_frame(uint32_t id, http2_frame* frame) {
    if(id >= stream_vector.size()) {
        return;
    }

    http2_stream& stream = stream_vector[id];

    switch(stream.get_status()) {
        case HTTP2_STREAM_IDLE : break;
        case HTTP2_STREAM_RESERVED : break;
        case HTTP2_STREAM_OPEN : break;
        case HTTP2_STREAM_HALF_CLOSED_LOCAL : break;
        case HTTP2_STREAM_HALF_CLOSED_REMOTE : break;
        case HTTP2_STREAM_CLOSED : break;
        default : break;
    }

    frame->send_frame(fd);
}

uint32_t http2_connection::get_last_server_stream_id() {
    return stream_vector.size();
}

uint32_t http2_connection::get_last_client_stream_id() {
    return stream_vector.size();
}

HTTP2_STREAM_STATUS http2_connection::get_stream_status(int id) {
    if(id <= 1)
        return HTTP2_STREAM_RESERVED;
    if(id >= stream_vector.size())
        return HTTP2_STREAM_IDLE;
    return stream_vector[id].get_status();
}