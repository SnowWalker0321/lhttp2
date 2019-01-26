#ifndef _LHTTP2_SETTINGS_H_
#define _LHTTP2_SETTINGS_H_

#include <stdint.h>

namespace lhttp2 {
    class Settings {
    public:
        const uint32_t HeaderTableSize() const;
        const bool EnablePush() const;
        const uint32_t MaxConcurrentStream() const;
        const uint32_t InitialWindowSize() const;
        const uint32_t MaxFrameSize() const;
        const uint32_t MaxHeaderListSize() const;

        void SetHeaderTableSize(uint32_t header_table_size);
        void SetEnablePush(bool enable_push);
        void SetMaxConcurrentStream(uint32_t max_concurrent_stream);
        void SetInitialWindowSize(uint32_t initial_window_size);
        void SetMaxFrameSize(uint32_t max_frame_size);
        void SetMaxHeaderListSize(uint32_t max_header_list_size);

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