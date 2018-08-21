#ifndef _HTTP2_SETTINGS_H_
#define _HTTP2_SETTINGS_H_

#include <stdint.h>

/*
    Structure for storing HTTP/2 settings.
*/
struct http2_settings {
    uint32_t header_table_size = 0x1000;
    bool enable_push = true;
    uint32_t max_concurrent_stream = UINT32_MAX;
    uint32_t initial_window_size = 0xFFFF;
    uint32_t max_frame_size = 0x4000;
    uint32_t max_header_list_size = UINT32_MAX;
};

#endif