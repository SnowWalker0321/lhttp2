#include "settings.h"

using namespace lhttp2;

const uint32_t Settings::header_table_size() const {
    return header_table_size_;
}

const bool Settings::enable_push() const {
    return enable_push_;

}
const uint32_t Settings::max_concurrent_stream() const {
    return max_concurrent_stream_;
}

const uint32_t Settings::initial_window_size() const {
    return initial_window_size_;
}

const uint32_t Settings::max_frame_size() const {
    return max_frame_size_;
}

const uint32_t Settings::max_header_list_size() const {
    return max_header_list_size_;
}

void Settings::set_header_table_size(uint32_t header_table_size) {
    header_table_size_ = header_table_size;
}

void Settings::set_enable_push(bool enable_push) {
    enable_push_ = enable_push;
}

void Settings::set_max_concurrent_stream(uint32_t max_concurrent_stream) {
    max_concurrent_stream_ = max_concurrent_stream;
}

void Settings::set_initial_window_size(uint32_t initial_window_size) {
    initial_window_size_ = initial_window_size;
}

void Settings::set_max_frame_size(uint32_t max_frame_size) {
    max_frame_size_ = max_frame_size;
}

void Settings::set_max_header_list_size(uint32_t max_header_list_size) {
    max_header_list_size_ = max_header_list_size;
}