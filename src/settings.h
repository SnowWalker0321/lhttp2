#ifndef _LHTTP2_SETTINGS_H_
#define _LHTTP2_SETTINGS_H_

#include <stdint.h>

namespace lhttp2 {
    class Settings {
    public:
        const uint32_t header_table_size() const;
        const bool enable_push() const;
        const uint32_t max_concurrent_stream() const;
        const uint32_t initial_window_size() const;
        const uint32_t max_frame_size() const;
        const uint32_t max_header_list_size() const;

        void set_header_table_size(uint32_t header_table_size);
        void set_enable_push(bool enable_push);
        void set_max_concurrent_stream(uint32_t max_concurrent_stream);
        void set_initial_window_size(uint32_t initial_window_size);
        void set_max_frame_size(uint32_t max_frame_size);
        void set_max_header_list_size(uint32_t max_header_list_size);

    private:
        uint32_t header_table_size_ = 0x1000;
        bool enable_push_ = true;
        uint32_t max_concurrent_stream_ = UINT32_MAX;
        uint32_t initial_window_size_ = 0xFFFF;
        uint32_t max_frame_size_ = 0x4000;
        uint32_t max_header_list_size_ = UINT32_MAX;
    };
}

#endif