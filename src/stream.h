#ifndef _HTTP2_STREAM_H_
#define _HTTP2_STREAM_H_

#include <stdint.h>

#include "frame.h"

namespace lhttp2 {
    class Stream {
    public:
        typedef enum _HTTP2_STREAM_STATUS {
            HTTP2_STREAM_IDLE,
            HTTP2_STREAM_RESERVED,
            HTTP2_STREAM_OPEN,
            HTTP2_STREAM_HALF_CLOSED_LOCAL,
            HTTP2_STREAM_HALF_CLOSED_REMOTE,
            HTTP2_STREAM_CLOSED,
        } HTTP2_STREAM_STATUS;

        Stream();
        ~Stream();

        const HTTP2_STREAM_STATUS Status() const;
        void SetStatus(HTTP2_STREAM_STATUS status);

    private:
        HTTP2_STREAM_STATUS status_;
        uint32_t window_size_ = 65535;
    };
}

#endif