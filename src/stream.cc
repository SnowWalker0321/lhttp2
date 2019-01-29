#include "stream.h"

using namespace lhttp2;

Stream::Stream() : status_(HTTP2_STREAM_IDLE) {
}

Stream::~Stream() {
}

const Stream::HTTP2_STREAM_STATUS Stream::status() const {
    return status_;
}

void Stream::set_status(HTTP2_STREAM_STATUS status) {
    status_ = status;
}