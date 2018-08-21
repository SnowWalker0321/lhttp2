#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <errno.h>

#include "http2_frame.h"

const char preface[] = PREFACE;

/*
    ### Implementation of DATA FRAME ###
*/
http2_data_frame::http2_data_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length = GET_1B_INT(buff + idx, 0xFF);
        idx = idx + 1;
    }

    data.copy_buffer(buff + idx, len - pad_length - idx, 0);
}

Buffer* http2_data_frame::get_frame_payload_stream() {
    int idx = 0;
    Buffer *stream = new Buffer(pad_length + data.len);

    if(has_padded_flag()) {
        (*stream)[0] = pad_length;
        idx = 1;
    }

    stream->copy_buffer(data, idx);

    return stream;
}

/*
    ### Implementation of HEADERS FRAME ###
*/
http2_headers_frame::http2_headers_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
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
}

Buffer* http2_headers_frame::get_frame_payload_stream() {
    int idx = 0;
    Buffer *stream = new Buffer(header.len + pad_length + ((has_priority_flag()) ? 5 : 0));

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

/*
    ### Implementation of PRIORITY FRAME ###
*/
http2_priority_frame::http2_priority_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    exclusive = GET_1BIT(buff, 0x01);
    stream_dependency = GET_4B_INT(buff, 0x7FFFFFFF);
    weight = GET_1B_INT(buff + 4, 0xFF);
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

/*
    ### Implementation of RST_STREAM FRAME ###
*/
http2_rst_stream_frame::http2_rst_stream_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    error_code = GET_4B_INT(buff, 0xFFFFFFFF);
}

Buffer* http2_rst_stream_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(32);

    SET_4B_INT(&(*stream)[0], error_code, 0xFFFFFFFF);

    return stream;
}

/*
    ### Implementation of SETTINGS FRAME ###
*/
http2_settings_frame::http2_settings_frame(const char* buff, const int len, http2_frame* header) : http2_frame_payload(header) {
    if(len % 6 != 0) throw std::invalid_argument("Invalid settings buffer");

    int i, set_cnt = len / 6;
    unsigned int id, val;

    for(i = 0; i < set_cnt; i++) {
        id = GET_2B_INT(buff + i * 6, 0xFFFF);
        val = GET_4B_INT(buff + i * 6 + 2, 0xFFFFFFFF);
        if(id == SETTINGS_HEADER_TABLE_SIZE) settings.header_table_size = val;
        else if(id == SETTINGS_ENABLE_PUSH) settings.enable_push = val;
        else if(id == SETTINGS_MAX_CONCURRENT_STREAMS) settings.max_concurrent_stream = val;
        else if(id == SETTINGS_INITIAL_WINDOW_SIZE) settings.initial_window_size = val;
        else if(id == SETTINGS_MAX_FRAME_SIZE) {
            if(val < 0x4000) val = 0x4000;
            if(val > 0xFFFFFF) val = 0xFFFFFF;
            settings.max_frame_size = val;
        }
        else if(id == SETTINGS_MAX_HEADER_LIST_SIZE) settings.max_header_list_size = val;
    }
}

http2_settings_frame::http2_settings_frame(http2_settings set) {
    settings = set;
}

Buffer* http2_settings_frame::get_frame_payload_stream() {
    Buffer *stream;
    int set_cnt = 0, idx = 0, stream_len;

    if(has_ack_flag() == true) {
        return nullptr;
    }

    if(settings.header_table_size != 0x1000) set_cnt++;
    if(settings.enable_push != true) set_cnt++;
    if(settings.max_concurrent_stream != UINT32_MAX) set_cnt++;
    if(settings.initial_window_size != 0xFFFF) set_cnt++;
    if(settings.max_frame_size != 0x4000) set_cnt++;
    if(settings.max_header_list_size != UINT32_MAX) set_cnt++;
    
    if(set_cnt == 0) {
        return nullptr;
    }

    stream = new Buffer(set_cnt * 6);

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

/*
    ### Implementation of PUSH_PROMISE FRAME ###
*/
http2_push_promise_frame::http2_push_promise_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length = GET_1B_INT(buff + idx, 0xFF);
        idx = idx + 1;
    }

    reserved = GET_1BIT(buff + idx, 0x01);
    promised_stream_id = GET_4B_INT(buff + idx, 0x7FFFFFFF);
    
    header_block_fragment.copy_buffer(buff + idx + 4, len - pad_length - idx, 0);
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

/*
    ### Implementation of PING FRAME ###
*/
http2_ping_frame::http2_ping_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    opaque_data = GET_8B_INT(buff, 0xFFFFFFFFFFFFFFFF);
}

Buffer* http2_ping_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(8);

    SET_8B_INT(&(*stream)[0], opaque_data, 0xFFFFFFFFFFFFFFFF);

    return stream;
}

/*
    ### Implementation of GOAWAY FRAME ###
*/
http2_goaway_frame::http2_goaway_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    reserved = GET_1BIT(buff, 0x01);
    last_stream_id = GET_4B_INT(buff, 0x7FFFFFFF);
    error_code = GET_4B_INT(buff + 4, 0xFFFFFFFF);

    additional_opaque_data.copy_buffer(buff + 8, len - 8, 0);
}

Buffer* http2_goaway_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(8 + additional_opaque_data.len);

    (*stream)[0] = (reserved << 7) | ((last_stream_id >> 24) & 0x7F);
    (*stream)[1] = (last_stream_id >> 16) & 0xFF;
    (*stream)[2] = (last_stream_id >> 8) & 0xFF;
    (*stream)[3] = last_stream_id & 0xFF;

    SET_4B_INT(&(*stream)[4], error_code, 0xFFFFFFFF);

    stream->copy_buffer(additional_opaque_data, 8);

    return stream;
}

/*
    ### Implementation of WINDOW_UPDATE FRAME ###
*/
http2_window_update_frame::http2_window_update_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    reserved = GET_1BIT(buff, 0x01);
    window_size_increment = GET_4B_INT(buff, 0x7FFFFFFF);
}

http2_window_update_frame::http2_window_update_frame(int size) {
    window_size_increment = (uint32_t)size;
    frame_header->set_length(4);
    frame_header->set_type(HTTP2_WINDOW_UPDATE_FRAME);
}

Buffer* http2_window_update_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(4);

    (*stream)[0] = (reserved << 7) | ((window_size_increment >> 24) & 0x7F);
    (*stream)[1] = (window_size_increment >> 16) & 0xFF;
    (*stream)[2] = (window_size_increment >> 8) & 0xFF;
    (*stream)[3] = window_size_increment & 0xFF;

    return stream;
}

/*
    ### Implementation of CONTINUATION FRAME ###
*/
http2_continuation_frame::http2_continuation_frame(const char* buff, const int len, http2_frame* frame_header) : http2_frame_payload(frame_header) {
    header_block_fragment.copy_buffer(buff, len, 0);
}

/*
    ### Implementation of FRAME_PAYLOAD FRAME ###
*/
Buffer* http2_continuation_frame::get_frame_payload_stream() {
    Buffer *stream = new Buffer(header_block_fragment);
    return stream;
}

http2_frame::http2_frame(const char *buff, const int len) {
    parse_frame_header(buff, 9);
    parse_frame_payload(buff + 9, len - 9);
}

http2_frame::http2_frame(const int fd) {
    recv_frame(fd);
}

http2_frame::~http2_frame() {
    if(frame_payload != nullptr) {
        if(type == HTTP2_DATA_FRAME)
            delete (http2_data_frame*)frame_payload;
        else if(type == HTTP2_HEADERS_FRAME)
            delete (http2_headers_frame*)frame_payload;
        else if(type == HTTP2_PRIORTY_FRAME)
            delete (http2_priority_frame*)frame_payload;
        else if(type == HTTP2_RST_STREAM_FRAME)
            delete (http2_rst_stream_frame*)frame_payload;
        else if(type == HTTP2_SETTINGS_FRAME)
            delete (http2_settings_frame*)frame_payload;
        else if(type == HTTP2_PUSH_PROMISE_FRAME)
            delete (http2_push_promise_frame*)frame_payload;
        else if(type == HTTP2_PING_FRAME)
            delete (http2_ping_frame*)frame_payload;
        else if(type == HTTP2_GOAWAY_FRAME)
            delete (http2_goaway_frame*)frame_payload;
        else if(type == HTTP2_WINDOW_UPDATE_FRAME)
            delete (http2_window_update_frame*)frame_payload;
        else if(type == HTTP2_CONTINUATION_FRAME)
            delete (http2_continuation_frame*)frame_payload;
        else
            frame_payload = nullptr;
    }
}

Buffer* http2_frame::get_frame_stream() {
    Buffer* header_stream = new Buffer(9);
    Buffer* payload_stream = nullptr;

    if(frame_payload != nullptr && (payload_stream = frame_payload->get_frame_payload_stream()) != nullptr) {
        length = payload_stream->len;
        *header_stream += *payload_stream;
        delete payload_stream;
    } else {
        length = 0;
    }

    SET_3B_INT(header_stream->buffer, length, 0xFFFFFF);
    SET_1B_INT(header_stream->buffer + 3, (uint8_t)type, 0xFF);
    SET_1B_INT(header_stream->buffer + 4, flags, 0xFF);
    SET_4B_INT(header_stream->buffer + 5, stream_id, 0x7FFFFFFF);
    header_stream->buffer[5] = (reserved << 7) | header_stream->buffer[5];

    return header_stream;
}

int http2_frame::recv_frame(const int fd) {
    char header_buff[9];
    char* payload_buff = nullptr;
    int read_len;

    read_len = ::read(fd, header_buff, 9);
    if(read_len != 9) {
        return 0;
    }

    parse_frame_header((const char *)header_buff, 9);

    payload_buff = (char *)malloc(sizeof(char) * length);
    read_len = ::read(fd, payload_buff, length);
    if(read_len != length) return 0;

    parse_frame_payload((const char *)payload_buff, length);

    if(payload_buff) free(payload_buff);

    return read_len + 9;
}

int http2_frame::send_frame(const int fd) {
    Buffer *stream = get_frame_stream();
    signal(SIGPIPE, SIG_IGN);
    int len = ::send(fd, stream->buffer, stream->len, 0);
    delete stream;
    return len;
}

void http2_frame::parse_frame_header(const char* buff, int len) {
    int i, idx = 0;

    if(buff == nullptr) throw std::invalid_argument("Null Pointer buffer");
    if(len != 9) throw std::invalid_argument("Invalid frame buffer");

    length = GET_3B_INT(buff + idx, 0x00FFFFFF);
    type = (HTTP2_FRAME_TYPE)GET_1B_INT(buff + idx + 3, 0xFF);
    flags = GET_1B_INT(buff + idx + 4, 0xFF);
    reserved = GET_1BIT(buff + idx + 5, 0x01);
    stream_id = GET_4B_INT(buff + idx + 5, 0x7FFFFFFF);

    if(type > 0x09) throw std::invalid_argument("Invalid frame type");
}

void http2_frame::parse_frame_payload(const char* buff, int len) {
    if(type == HTTP2_DATA_FRAME)
        frame_payload = new http2_data_frame(buff, len, this);
    else if(type == HTTP2_HEADERS_FRAME)
        frame_payload = new http2_headers_frame(buff, len, this);
    else if(type == HTTP2_PRIORTY_FRAME)
        frame_payload = new http2_priority_frame(buff, len, this);
    else if(type == HTTP2_RST_STREAM_FRAME)
        frame_payload = new http2_rst_stream_frame(buff, len, this);
    else if(type == HTTP2_SETTINGS_FRAME)
        frame_payload = new http2_settings_frame(buff, len, this);
    else if(type == HTTP2_PUSH_PROMISE_FRAME)
        frame_payload = new http2_push_promise_frame(buff, len, this);
    else if(type == HTTP2_PING_FRAME)
        frame_payload = new http2_ping_frame(buff, len, this);
    else if(type == HTTP2_GOAWAY_FRAME)
        frame_payload = new http2_goaway_frame(buff, len, this);
    else if(type == HTTP2_WINDOW_UPDATE_FRAME)
        frame_payload = new http2_window_update_frame(buff, len, this);
    else if(type == HTTP2_CONTINUATION_FRAME)
        frame_payload = new http2_continuation_frame(buff, len, this);
    else
        frame_payload = nullptr;
}

int http2_frame_payload::send_frame(const int fd) {
    return frame_header->send_frame(fd);
}

void http2_send_preface(int fd) {
    ::send(fd, preface, PREFACE_LEN, 0);
}

bool http2_check_preface(const char* buffer, int len) {
    if(len != PREFACE_LEN) return false;

    for(int i = 0; i < PREFACE_LEN; i++)
        if(preface[i] != buffer[i])
            return false;

    return true;
}

bool http2_check_preface(int fd) {
    char buffer[PREFACE_LEN];
    int read_len;

    read_len = ::read(fd, buffer, PREFACE_LEN);
    if(read_len != PREFACE_LEN) return false;

    return http2_check_preface(buffer, read_len);
}

http2_frame* http2_get_frame(int fd) {
    http2_frame *frame = new http2_frame(fd);
    return frame;
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
    return "ERROR";
}