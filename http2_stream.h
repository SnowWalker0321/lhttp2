#ifndef _HTTP2_STREAM_H_
#define _HTTP2_STREAM_H_

#include <stdint.h>

#include "http2_frame.h"

typedef enum _HTTP2_STREAM_STATUS {
    HTTP2_STREAM_IDLE,
    HTTP2_STREAM_RESERVED,
    HTTP2_STREAM_OPEN,
    HTTP2_STREAM_HALF_CLOSED_LOCAL,
    HTTP2_STREAM_HALF_CLOSED_REMOTE,
    HTTP2_STREAM_CLOSED,
} HTTP2_STREAM_STATUS;

class http2_stream {
public:
    http2_stream();
    ~http2_stream();

    HTTP2_STREAM_STATUS get_status() { return status; }

    void set_status(HTTP2_STREAM_STATUS s) { status = s; }

private:
    HTTP2_STREAM_STATUS status;
    uint32_t window_size = 65535;
};

#endif