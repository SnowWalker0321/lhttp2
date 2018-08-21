#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include "http2_frame.h"

/*
    Implementation of frame header
*/
http2_frame::~http2_frame() {
}

uint32_t http2_frame::get_length() {
    return length;
}

HTTP2_FRAME_TYPE http2_frame::get_type() {
    return type;
}

uint8_t http2_frame::get_flags() {
    return flags;
}

uint32_t http2_frame::get_stream_id() {
    return stream_id;
}

bool http2_frame::get_reserved() {
    return reserved;
}

void http2_frame::set_flags(uint8_t f) {
    flags = flags | f;
}

void http2_frame::clear_flags(uint8_t f) {
    flags = flags & ~f;
}

bool http2_frame::has_flags(uint8_t f) {
    return ((flags & f) == f);
}

void http2_frame::set_stream_id(uint32_t id) {
    stream_id = id;
}

Buffer* http2_frame::get_frame_stream() {
    Buffer* header_stream = new Buffer(9 + length);
    Buffer* payload_stream = nullptr;

    SET_3B_INT(header_stream->buffer, length, 0xFFFFFF);
    SET_1B_INT(header_stream->buffer + 3, (uint8_t)type, 0xFF);
    SET_1B_INT(header_stream->buffer + 4, flags, 0xFF);
    SET_4B_INT(header_stream->buffer + 5, stream_id, 0x7FFFFFFF);
    header_stream->buffer[5] = (reserved << 7) | header_stream->buffer[5];

    if(length > 0) {
        payload_stream = get_frame_payload_stream();
        if(payload_stream != nullptr) {
            header_stream->copy_buffer(*payload_stream, 9);
            delete payload_stream;
        }
    }

    return header_stream;
}

int http2_frame::send_frame(const int fd) {
    Buffer *stream = get_frame_stream();
    signal(SIGPIPE, SIG_IGN);
    int len = ::send(fd, stream->buffer, stream->len, 0);
    delete stream;
    return len;
}

/*
    Implementation of DATA FRAME
*/
http2_data_frame::http2_data_frame() {
    type = HTTP2_DATA_FRAME;
}

http2_data_frame::http2_data_frame(Buffer d, uint8_t p_len) {
    http2_data_frame();

    pad_length = p_len;
    if(pad_length > 0) set_flags(HTTP2_FLAG_PADDED);
    else clear_flags(HTTP2_FLAG_PADDED);

    data = d;

    update_length();
}

http2_data_frame::~http2_data_frame() {
}

uint8_t http2_data_frame::get_pad_length() {
    return pad_length;
}

Buffer http2_data_frame::get_data() {
    return data;
}

void http2_data_frame::set_pad_length(uint8_t p_len) {
    pad_length = p_len;
}

void http2_data_frame::set_data(Buffer& d) {
    data = d;
    update_length();
}

bool http2_data_frame::has_end_stream_flag() {
    return has_flags(HTTP2_FLAG_END_STREAM);
}

bool http2_data_frame::has_padded_flag() {
    return has_flags(HTTP2_FLAG_PADDED);
}

void http2_data_frame::set_end_stream_flag() {
    set_flags(HTTP2_FLAG_END_STREAM);
}

void http2_data_frame::set_padded_flag() {
    set_flags(HTTP2_FLAG_PADDED);
    update_length();
}

void http2_data_frame::clear_end_stream_flag() {
    clear_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_data_frame::clear_padded_flag() {
    clear_flags(HTTP2_FLAG_PADDED);
    update_length();
}

Buffer* http2_data_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(length);

    if(has_padded_flag()) {
        (*stream)[0] = pad_length;
        stream->copy_buffer(data, 1);
    }
    else {
        stream->copy_buffer(data, 0);
    }

    return stream;
}

bool http2_data_frame::parse_frame_payload(const char* buff, const int len) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length = GET_1B_INT(buff + idx, 0xFF);
        idx = idx + 1;
    }

    data.copy_buffer(buff + idx, len - pad_length - idx, 0);
    update_length();

    return true;
}

void http2_data_frame::update_length() {
    length = data.len;

    if(has_padded_flag())
        length = length + pad_length + 1;
}

/*
    Implementation of HEADERS FRAME
*/
http2_headers_frame::http2_headers_frame() {
    type = HTTP2_HEADERS_FRAME;
}

http2_headers_frame::http2_headers_frame(Buffer h, uint8_t p_len) {
    http2_headers_frame();

    pad_length = p_len;
    if(pad_length > 0) set_flags(HTTP2_FLAG_PADDED);
    else clear_flags(HTTP2_FLAG_PADDED);

    clear_flags(HTTP2_FLAG_PRIORITY);

    header = h;

    update_length();
}

http2_headers_frame::http2_headers_frame(Buffer h, bool e, uint32_t s_dep, uint8_t w, uint8_t p_len) {
    http2_headers_frame();

    pad_length = p_len;
    if(pad_length > 0) set_flags(HTTP2_FLAG_PADDED);
    else clear_flags(HTTP2_FLAG_PADDED);

    set_flags(HTTP2_FLAG_PRIORITY);
    exclusive = e;
    stream_dependency = s_dep;
    weight = w;

    header = h;

    update_length();
}

http2_headers_frame::~http2_headers_frame() {
}

uint8_t http2_headers_frame::get_pad_length() {
    return pad_length;
}

bool http2_headers_frame::get_exclusive() { 
    return exclusive;
}

uint32_t http2_headers_frame::get_stream_dependency() {
    return stream_dependency;
}

uint8_t http2_headers_frame::get_weight() {
    return weight;
}

Buffer http2_headers_frame::get_header() {
    return header;
}

void http2_headers_frame::set_pad_length(uint8_t p_len) {
    pad_length = p_len;
}

void http2_headers_frame::set_exclusive(bool e) {
    exclusive = e;
}

void http2_headers_frame::set_stream_dependency(uint32_t dep) {
    stream_dependency = dep;
}

void http2_headers_frame::set_weight(uint8_t w) {
    weight = w;
}

void http2_headers_frame::set_header(Buffer& h) {
    header = h;
    update_length();
}

bool http2_headers_frame::has_end_stream_flag() {
    return has_flags(HTTP2_FLAG_END_STREAM);
}

bool http2_headers_frame::has_end_headers_flag() {
    return has_flags(HTTP2_FLAG_END_HEADERS);
}

bool http2_headers_frame::has_padded_flag() {
    return has_flags(HTTP2_FLAG_PADDED);
}

bool http2_headers_frame::has_priority_flag() {
    return has_flags(HTTP2_FLAG_PRIORITY);
}

void http2_headers_frame::set_end_stream_flag() {
    set_flags(HTTP2_FLAG_END_STREAM);
}

void http2_headers_frame::set_end_headers_flag() {
    set_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_headers_frame::set_padded_flag() {
    set_flags(HTTP2_FLAG_PADDED);
    update_length();
}

void http2_headers_frame::set_priority_flag() {
    set_flags(HTTP2_FLAG_PRIORITY);
    update_length();
}

void http2_headers_frame::clear_end_stream_flag() {
    clear_flags(HTTP2_FLAG_END_STREAM);
}

void http2_headers_frame::clear_end_headers_flag() {
    clear_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_headers_frame::clear_padded_flag() {
    clear_flags(HTTP2_FLAG_PADDED);
    update_length();
}

void http2_headers_frame::clear_priority_flag() {
    clear_flags(HTTP2_FLAG_PRIORITY);
    update_length();
}

Buffer* http2_headers_frame::get_frame_payload_stream() {
    int idx = 0;
    Buffer *stream = new Buffer(length);

    if(has_padded_flag()) {
        (*stream)[idx] = pad_length;
        idx = idx + 1;
    }

    if(has_priority_flag()) {
        (*stream)[idx] = (exclusive << 7) | ((stream_dependency >> 24) & 0x7F);
        (*stream)[idx + 1] = (stream_dependency >> 16) & 0xFF;
        (*stream)[idx + 2] = (stream_dependency >> 8) & 0xFF;
        (*stream)[idx + 3] = stream_dependency & 0xFF;
        (*stream)[idx + 4] = weight & 0xFF;
        idx = idx + 5;
    }

    stream->copy_buffer(header, idx);

    return stream;
}

bool http2_headers_frame::parse_frame_payload(const char* buff, const int len) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length = GET_1B_INT(buff + idx, 0xFF);
        idx = idx + 1;
    }

    if(has_priority_flag()) {
        exclusive = GET_1BIT(buff + idx, 0x01);
        stream_dependency = GET_4B_INT(buff + idx, 0x7FFFFFFF);
        weight = GET_1B_INT(buff + idx + 4, 0xFF);
        idx = idx + 5;
    }
    
    header.copy_buffer(buff + idx, len - pad_length - idx, 0);
    update_length();

    return true;
}

void http2_headers_frame::update_length() {
    length = header.len;

    if(has_padded_flag())
        length = length + pad_length + 1;

    if(has_priority_flag())
        length = length + 5;
}

/*
    Implementation of PRIORITY FRAME
*/
http2_priority_frame::http2_priority_frame() {
    type = HTTP2_PRIORTY_FRAME;
    length = 5;
}

http2_priority_frame::http2_priority_frame(bool e, uint32_t s_dep, uint8_t w) {
    http2_priority_frame();
    exclusive = e;
    stream_dependency = s_dep;
    weight = w;
}

http2_priority_frame::~http2_priority_frame() {
}

bool http2_priority_frame::get_exclusive() {
    return exclusive;
}

uint32_t http2_priority_frame::get_stream_dependency() {
    return stream_dependency;
}

uint8_t http2_priority_frame::get_weight() {
    return weight;
}

void http2_priority_frame::set_exclusive(bool e) {
    exclusive = e;
}

void http2_priority_frame::set_stream_dependency(uint32_t dep) {
    stream_dependency = dep;
}

void http2_priority_frame::set_weight(uint8_t w) {
    weight = w;
}

Buffer* http2_priority_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(40);

    (*stream)[0] = (exclusive << 7) | ((stream_dependency >> 24) & 0x7F);
    (*stream)[1] = (stream_dependency >> 16) & 0xFF;
    (*stream)[2] = (stream_dependency >> 8) & 0xFF;
    (*stream)[3] = stream_dependency & 0xFF;
    (*stream)[4] = weight & 0xFF;

    return stream;
}

bool http2_priority_frame::parse_frame_payload(const char* buff, const int len) {
    if(len != 5) return false;

    exclusive = GET_1BIT(buff, 0x01);
    stream_dependency = GET_4B_INT(buff, 0x7FFFFFFF);
    weight = GET_1B_INT(buff + 4, 0xFF);

    update_length();

    return true;
}

void http2_priority_frame::update_length() {
    length = 5;
}

/*
    Implementation of RST_STREAM FRAME
*/
http2_rst_stream_frame::http2_rst_stream_frame() {
    type = HTTP2_RST_STREAM_FRAME;
    length = 4;
}

http2_rst_stream_frame::http2_rst_stream_frame(uint32_t code) {
    http2_rst_stream_frame();
    error_code = code;
}

http2_rst_stream_frame::~http2_rst_stream_frame() {
}

bool http2_rst_stream_frame::get_error_code() {
    return error_code;
}

void http2_rst_stream_frame::set_error_code(uint32_t code) {
    error_code = code;
}

Buffer* http2_rst_stream_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(32);

    SET_4B_INT(&(*stream)[0], error_code, 0xFFFFFFFF);

    update_length();

    return stream;
}

bool http2_rst_stream_frame::parse_frame_payload(const char* buff, const int len) {
    if(len != 4) return false;

    error_code = GET_4B_INT(buff, 0xFFFFFFFF);

    return true;
}

void http2_rst_stream_frame::update_length() {
    length = 4;
}

/*
    Implementation of SETTINGS FRAME
*/
http2_settings_frame::http2_settings_frame() {
    type = HTTP2_SETTINGS_FRAME;
}

http2_settings_frame::http2_settings_frame(http2_settings set) {
    http2_settings_frame();
    settings = set;
    update_length();
}

http2_settings_frame::~http2_settings_frame() {
}

http2_settings http2_settings_frame::get_settings() {
    return settings;
}

void http2_settings_frame::set_settings(http2_settings set) {
    settings = set;
    update_length();
}

bool http2_settings_frame::has_ack_flag() {
    return has_flags(HTTP2_FLAG_ACK);
}

void http2_settings_frame::set_ack_flag() {
    set_flags(HTTP2_FLAG_ACK);
    update_length();
}

void http2_settings_frame::clear_ack_flag() {
    clear_flags(HTTP2_FLAG_ACK);
    update_length();
}

Buffer* http2_settings_frame::get_frame_payload_stream() {
    if(has_ack_flag() == true || length == 0) {
        return nullptr;
    }

    int idx = 0;
    Buffer *stream = new Buffer(length);

    if(settings.header_table_size != 0x1000) {
        SET_2B_INT(stream->buffer + idx, 0x0001, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.header_table_size, 0xFFFFFFFF);
        idx = idx + 6;
    }

    if(settings.enable_push != true) {
        SET_2B_INT(stream->buffer + idx, 0x0002, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.enable_push, 0xFFFFFFFF);
        idx = idx + 6;
    }

    if(settings.max_concurrent_stream != UINT32_MAX) {
        SET_2B_INT(stream->buffer + idx, 0x0003, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.max_concurrent_stream, 0xFFFFFFFF);
        idx = idx + 6;
    }

    if(settings.initial_window_size != 0xFFFF) {
        SET_2B_INT(stream->buffer + idx, 0x0004, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.initial_window_size, 0xFFFFFFFF);
        idx = idx + 6;
    }

    if(settings.max_frame_size != 0x4000) {
        SET_2B_INT(stream->buffer + idx, 0x0005, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.max_frame_size, 0xFFFFFFFF);
        idx = idx + 6;
    }

    if(settings.max_header_list_size != UINT32_MAX) {
        SET_2B_INT(stream->buffer + idx, 0x0006, 0xFFFF);
        SET_4B_INT(stream->buffer + idx + 2, settings.max_header_list_size, 0xFFFFFFFF);
        idx = idx + 6;
    }
    
    return stream;
}

bool http2_settings_frame::parse_frame_payload(const char* buff, const int len) {
    if(len % 6 != 0) return false;

    int i, set_cnt = len / 6;
    unsigned int id, val;
    http2_settings set;

    for(i = 0; i < set_cnt; i++) {
        id = GET_2B_INT(buff + i * 6, 0xFFFF);
        val = GET_4B_INT(buff + i * 6 + 2, 0xFFFFFFFF);
        if(id == SETTINGS_HEADER_TABLE_SIZE) set.header_table_size = val;
        else if(id == SETTINGS_ENABLE_PUSH) set.enable_push = val;
        else if(id == SETTINGS_MAX_CONCURRENT_STREAMS) set.max_concurrent_stream = val;
        else if(id == SETTINGS_INITIAL_WINDOW_SIZE) set.initial_window_size = val;
        else if(id == SETTINGS_MAX_FRAME_SIZE) {
            if(val < 0x4000) val = 0x4000;
            if(val > 0xFFFFFF) val = 0xFFFFFF;
            set.max_frame_size = val;
        }
        else if(id == SETTINGS_MAX_HEADER_LIST_SIZE) set.max_header_list_size = val;
    }

    settings = set;
    update_length();

    return true;
}

void http2_settings_frame::update_length() {
    if(has_ack_flag()) {
        length = 0;
        return;
    }

    int set_cnt = 0;

    if(settings.header_table_size != 0x1000) set_cnt++;
    if(settings.enable_push != true) set_cnt++;
    if(settings.max_concurrent_stream != UINT32_MAX) set_cnt++;
    if(settings.initial_window_size != 0xFFFF) set_cnt++;
    if(settings.max_frame_size != 0x4000) set_cnt++;
    if(settings.max_header_list_size != UINT32_MAX) set_cnt++;

    length = set_cnt * 6;
}

/*
    Implementation of PUSH_PROMISE FRAME
*/
http2_push_promise_frame::http2_push_promise_frame() {
    type = HTTP2_PUSH_PROMISE_FRAME;
    length = 4;
}

http2_push_promise_frame::http2_push_promise_frame(uint32_t stream_id, Buffer header, uint8_t p_len) {
    http2_push_promise_frame();

    promised_stream_id = stream_id;
    header_block_fragment = header;

    pad_length = p_len;
    if(pad_length > 0) set_flags(HTTP2_FLAG_PADDED);
    else clear_flags(HTTP2_FLAG_PADDED);

    update_length();
}

http2_push_promise_frame::~http2_push_promise_frame() {
}

uint8_t http2_push_promise_frame::get_pad_length() {
    return pad_length;
}

bool http2_push_promise_frame::get_reserved() {
    return reserved;
}

uint32_t http2_push_promise_frame::get_promised_stream_id() {
    return promised_stream_id;
}

Buffer http2_push_promise_frame::get_header_block_fragment() {
    return header_block_fragment;
}

void http2_push_promise_frame::set_pad_length(uint8_t p_len) {
    pad_length = p_len;
}

void http2_push_promise_frame::set_reserved(bool r) {
    reserved = r;
}

void http2_push_promise_frame::set_promised_stream_id(uint32_t id) {
    promised_stream_id = id;
}

void http2_push_promise_frame::set_header_block_fragment(Buffer header) {
    header_block_fragment = header;
    update_length();
}

bool http2_push_promise_frame::has_end_headers_flag() {
    return has_flags(HTTP2_FLAG_END_HEADERS);
}

bool http2_push_promise_frame::has_padded_flag() {
    return has_flags(HTTP2_FLAG_PADDED);
}

void http2_push_promise_frame::set_end_headers_flag() {
    set_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_push_promise_frame::set_padded_flag() {
    set_flags(HTTP2_FLAG_PADDED);
    update_length();
}

void http2_push_promise_frame::clear_end_headers_flag() {
    clear_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_push_promise_frame::clear_padded_flag() {
    clear_flags(HTTP2_FLAG_PADDED);
    update_length();
}

Buffer* http2_push_promise_frame::get_frame_payload_stream() {
    int idx = 0;
    Buffer *stream = new Buffer(pad_length + header_block_fragment.len);

    if(has_padded_flag()) {
        (*stream)[0] = pad_length;
        idx = 1;
    }

    (*stream)[idx] = (reserved << 7) | ((promised_stream_id >> 24) & 0x7F);
    (*stream)[idx + 1] = (promised_stream_id >> 16) & 0xFF;
    (*stream)[idx + 2] = (promised_stream_id >> 8) & 0xFF;
    (*stream)[idx + 3] = promised_stream_id & 0xFF;

    stream->copy_buffer(header_block_fragment, idx);

    return stream;
}

bool http2_push_promise_frame::parse_frame_payload(const char* buff, const int len) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length = GET_1B_INT(buff + idx, 0xFF);
        idx = idx + 1;
    }

    reserved = GET_1BIT(buff + idx, 0x01);
    promised_stream_id = GET_4B_INT(buff + idx, 0x7FFFFFFF);
    
    header_block_fragment.copy_buffer(buff + idx + 4, len - pad_length - idx, 0);
    update_length();

    return true;
}

void http2_push_promise_frame::update_length() {
    length = 4 + header_block_fragment.len;

    if(has_padded_flag())
        length = length + pad_length + 1;
}

/*
    Implementation of PING FRAME
*/
http2_ping_frame::http2_ping_frame() {
    type = HTTP2_PING_FRAME;
    length = 8;
}

http2_ping_frame::http2_ping_frame(uint64_t data) {
    http2_ping_frame();
    opaque_data = data;
}

http2_ping_frame::~http2_ping_frame() {
}

uint64_t http2_ping_frame::get_opaque_data() {
    return opaque_data;
}

void http2_ping_frame::set_opaque_data(uint64_t data) {
    opaque_data = data;
}

bool http2_ping_frame::has_ack_flag() {
    return has_flags(HTTP2_FLAG_ACK);
}

void http2_ping_frame::set_ack_flag() {
    set_flags(HTTP2_FLAG_ACK);
}

void http2_ping_frame::clear_ack_flag() {
    clear_flags(HTTP2_FLAG_ACK);
}

Buffer* http2_ping_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(8);

    SET_8B_INT(&(*stream)[0], opaque_data, 0xFFFFFFFFFFFFFFFF);

    return stream;
}

bool http2_ping_frame::parse_frame_payload(const char* buff, const int len) {
    opaque_data = GET_8B_INT(buff, 0xFFFFFFFFFFFFFFFF);

    update_length();

    return true;
}

void http2_ping_frame::update_length() {
    length = 8;
}

/*
    Implementation of GOAWAY FRAME
*/
http2_goaway_frame::http2_goaway_frame() {
    type = HTTP2_GOAWAY_FRAME;
    length = 8;
}

http2_goaway_frame::http2_goaway_frame(uint32_t stream_id, uint32_t code, Buffer debug_data) {
    http2_goaway_frame();

    last_stream_id = stream_id;
    error_code = code;
    additional_debug_data = debug_data;

    update_length();
}

http2_goaway_frame::~http2_goaway_frame() {
}

bool http2_goaway_frame::get_reserved() {
    return reserved;
}

uint32_t http2_goaway_frame::get_last_stream_id() {
    return last_stream_id;
}

uint32_t http2_goaway_frame::get_error_code() {
    return error_code;
}

Buffer http2_goaway_frame::get_additional_debug_data() {
    return additional_debug_data;
}

void http2_goaway_frame::set_reserved(bool r) {
    reserved = r;
}

void http2_goaway_frame::set_last_stream_id(uint32_t id) {
    last_stream_id = id;
}

void http2_goaway_frame::set_error_code(uint32_t code) {
    error_code = code;
}

void http2_goaway_frame::set_additional_debug_data(Buffer debug_data) {
    additional_debug_data = debug_data;
    update_length();
}

Buffer* http2_goaway_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(8 + additional_debug_data.len);

    (*stream)[0] = (reserved << 7) | ((last_stream_id >> 24) & 0x7F);
    (*stream)[1] = (last_stream_id >> 16) & 0xFF;
    (*stream)[2] = (last_stream_id >> 8) & 0xFF;
    (*stream)[3] = last_stream_id & 0xFF;

    SET_4B_INT(&(*stream)[4], error_code, 0xFFFFFFFF);

    stream->copy_buffer(additional_debug_data, 8);

    return stream;
}

bool http2_goaway_frame::parse_frame_payload(const char* buff, const int len) {
    reserved = GET_1BIT(buff, 0x01);
    last_stream_id = GET_4B_INT(buff, 0x7FFFFFFF);
    error_code = GET_4B_INT(buff + 4, 0xFFFFFFFF);

    additional_debug_data.copy_buffer(buff + 8, len - 8, 0);

    update_length();

    return true;
}

void http2_goaway_frame::update_length() {
    length = 8 + additional_debug_data.len;
}

/*
    Implementation of WINDOW_UPDATE FRAME
*/
http2_window_update_frame::http2_window_update_frame() {
    type = HTTP2_WINDOW_UPDATE_FRAME;
    length = 4;
}

http2_window_update_frame::http2_window_update_frame(uint32_t size) {
    http2_window_update_frame();
    window_size_increment = size;
}

http2_window_update_frame::~http2_window_update_frame() {
}

bool http2_window_update_frame::get_reserved() {
    return reserved;
}

uint32_t http2_window_update_frame::get_window_size_increment() {
    return window_size_increment;
}

void http2_window_update_frame::set_reserved(bool r) {
    reserved = r;
}

void http2_window_update_frame::set_window_size_increment(int size) {
    window_size_increment = size;
}

Buffer* http2_window_update_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(4);

    (*stream)[0] = (reserved << 7) | ((window_size_increment >> 24) & 0x7F);
    (*stream)[1] = (window_size_increment >> 16) & 0xFF;
    (*stream)[2] = (window_size_increment >> 8) & 0xFF;
    (*stream)[3] = window_size_increment & 0xFF;

    return stream;
}

bool http2_window_update_frame::parse_frame_payload(const char* buff, const int len) {
    reserved = GET_1BIT(buff, 0x01);
    window_size_increment = GET_4B_INT(buff, 0x7FFFFFFF);

    update_length();

    return true;
}

void http2_window_update_frame::update_length() {
    length = 4;
}

/*
    Implementation of CONTINUATION FRAME
*/
http2_continuation_frame::http2_continuation_frame() {
    type = HTTP2_CONTINUATION_FRAME;
}

http2_continuation_frame::http2_continuation_frame(Buffer header) {
    http2_continuation_frame();
    header_block_fragment = header;
    update_length();
}

http2_continuation_frame::~http2_continuation_frame() {
}

Buffer http2_continuation_frame::get_header_block_fragment() {
    return header_block_fragment;
}

void http2_continuation_frame::set_header_block_fragment(Buffer header) {
    header_block_fragment = header;
    update_length();
}

bool http2_continuation_frame::has_end_headers_flag() {
    return has_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_continuation_frame::set_end_headers_flag() {
    set_flags(HTTP2_FLAG_END_HEADERS);
}

void http2_continuation_frame::clear_end_headers_flag() {
    clear_flags(HTTP2_FLAG_END_HEADERS);
}

Buffer* http2_continuation_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(header_block_fragment);
    return stream;
}

bool http2_continuation_frame::parse_frame_payload(const char* buff, const int len) {
    header_block_fragment.copy_buffer(buff, len, 0);
    update_length();
    return true;
}

void http2_continuation_frame::update_length() {
    length = header_block_fragment.len;
}

/*
    Implementation of functions for HTTP/2 frame
*/
http2_frame* http2_recv_frame(const int fd) {
    if(fd < 0) {
        return nullptr;
    }

    http2_frame *frame;
    char header_buff[9];
    int length, flags, reserved, stream_id;
    HTTP2_FRAME_TYPE type;

    if(::read(fd, header_buff, 9) != 9) {
        return nullptr;
    }

    length = GET_3B_INT(header_buff, 0x00FFFFFF);
    type = (HTTP2_FRAME_TYPE)GET_1B_INT(header_buff + 3, 0xFF);
    flags = GET_1B_INT(header_buff + 4, 0xFF);
    reserved = GET_1BIT(header_buff + 5, 0x01);
    stream_id = GET_4B_INT(header_buff + 5, 0x7FFFFFFF);

    if(type > 0x09) {
        return nullptr;
    }

    char* payload_buff = new char[length];

    if(::read(fd, payload_buff, length) != length) {
        delete[] payload_buff;
        return nullptr;
    }

    if(type == HTTP2_DATA_FRAME)
        frame = new http2_data_frame();
    else if(type == HTTP2_HEADERS_FRAME)
        frame = new http2_headers_frame();
    else if(type == HTTP2_PRIORTY_FRAME)
        frame = new http2_priority_frame();
    else if(type == HTTP2_RST_STREAM_FRAME)
        frame = new http2_rst_stream_frame();
    else if(type == HTTP2_SETTINGS_FRAME)
        frame = new http2_settings_frame();
    else if(type == HTTP2_PUSH_PROMISE_FRAME)
        frame = new http2_push_promise_frame();
    else if(type == HTTP2_PING_FRAME)
        frame = new http2_ping_frame();
    else if(type == HTTP2_GOAWAY_FRAME)
        frame = new http2_goaway_frame();
    else if(type == HTTP2_WINDOW_UPDATE_FRAME)
        frame = new http2_window_update_frame();
    else if(type == HTTP2_CONTINUATION_FRAME)
        frame = new http2_continuation_frame();
    else {
        delete[] payload_buff;
        return nullptr;
    }

    frame->length = length;
    frame->type = type;
    frame->flags = flags;
    frame->reserved = reserved;
    frame->stream_id = stream_id;

    frame->parse_frame_payload((const char *)payload_buff, frame->length);

    delete[] payload_buff;

    return frame;
}

int http2_send_frame(http2_frame* frame, const int fd) {
    return frame->send_frame(fd);
}

const char* http2_get_frame_type_name(HTTP2_FRAME_TYPE type) {
    switch(type) {
        case HTTP2_DATA_FRAME: return "DATA";
        case HTTP2_HEADERS_FRAME: return "HEADERS";
        case HTTP2_PRIORTY_FRAME: return "PRIORITY";
        case HTTP2_RST_STREAM_FRAME: return "RST_STREAM";
        case HTTP2_SETTINGS_FRAME: return "SETTINGS";
        case HTTP2_PUSH_PROMISE_FRAME: return "PUSH_PROMISE";
        case HTTP2_PING_FRAME: return "PING";
        case HTTP2_GOAWAY_FRAME: return "GOAWAY";
        case HTTP2_WINDOW_UPDATE_FRAME: return "WINDOW_UPDATE";
        case HTTP2_CONTINUATION_FRAME: return "CONTINUATION";
        default: break;
    }
    return "UNKNOWN";
}