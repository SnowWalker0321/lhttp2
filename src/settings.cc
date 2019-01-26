#include "settings.h"

using namespace lhttp2;

const uint32_t Settings::HeaderTableSize() const {
    return header_table_size_;
}

const bool Settings::EnablePush() const {
    return enable_push_;

}
const uint32_t Settings::MaxConcurrentStream() const {
    return max_concurrent_stream_;
}

const uint32_t Settings::InitialWindowSize() const {
    return initial_window_size_;
}

const uint32_t Settings::MaxFrameSize() const {
    return max_frame_size_;
}

const uint32_t Settings::MaxHeaderListSize() const {
    return max_header_list_size_;
}

void Settings::SetHeaderTableSize(uint32_t header_table_size) {
    header_table_size_ = header_table_size;
}

void Settings::SetEnablePush(bool enable_push) {
    enable_push_ = enable_push;
}

void Settings::SetMaxConcurrentStream(uint32_t max_concurrent_stream) {
    max_concurrent_stream_ = max_concurrent_stream;
}

void Settings::SetInitialWindowSize(uint32_t initial_window_size) {
    initial_window_size_ = initial_window_size;
}

void Settings::SetMaxFrameSize(uint32_t max_frame_size) {
    max_frame_size_ = max_frame_size;
}

void Settings::SetMaxHeaderListSize(uint32_t max_header_list_size) {
    max_header_list_size_ = max_header_list_size;
}