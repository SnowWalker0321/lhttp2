#ifndef _HTTP2_FRAME_H_
#define _HTTP2_FRAME_H_

#include <stdint.h>
#include <iostream>

#include "Buffer.h"
#include "http2_settings.h"
#include "http2_error.h"

// The client connection preface starts with a sequence of 24 octets,
// "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define PREFACE "\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a"
#define PREFACE_LEN 24

class http2_frame;                  // Header of frame

/*
    Classes below are payload classes.
    All of them inherits http2_frame_payload class
    and is implemented according to the type of frame.
*/
typedef enum _HTTP2_FRAME_TYPE{
    HTTP2_DATA_FRAME = 0,
    HTTP2_HEADERS_FRAME,
    HTTP2_PRIORTY_FRAME,
    HTTP2_RST_STREAM_FRAME,
    HTTP2_SETTINGS_FRAME,
    HTTP2_PUSH_PROMISE_FRAME,
    HTTP2_PING_FRAME,
    HTTP2_GOAWAY_FRAME,
    HTTP2_WINDOW_UPDATE_FRAME,
    HTTP2_CONTINUATION_FRAME,
} HTTP2_FRAME_TYPE;

class http2_data_frame;             // DATA (0x0)
class http2_headers_frame;          // HEADERS (0x1)
class http2_priority_frame;         // PRIORITY (0x2)
class http2_rst_stream_frame;       // RST_STREAM (0x3)
class http2_settings_frame;         // SETTINGS (0x4)
class http2_push_promise_frame;     // PUSH_PROMISE (0x5)
class http2_ping_frame;             // PING (0x6)
class http2_goaway_frame;           // GOAWAY (0x7)
class http2_window_update_frame;    // WINDOW_UPDATE (0x8)
class http2_continuation_frame;     // CONTINUATION (0x9)

typedef enum _HTTP2_FRAME_FLAG {
    HTTP2_FLAG_ACK = 0x1,
    HTTP2_FLAG_END_STREAM = 0x1,
    HTTP2_FLAG_END_HEADERS = 0x4,
    HTTP2_FLAG_PADDED = 0x8,
    HTTP2_FLAG_PRIORITY = 0x20,
} HTTP2_FRAME_FLAG;

/*
    ### Header of frame ###

    All frames begin with a fixed 9-octet header followed by a variable-length payload.

    +-----------------------------------------------+
    |                 Length (24)                   |
    +---------------+---------------+---------------+
    |   Type (8)    |   Flags (8)   |
    +-+-------------+---------------+-------------------------------+
    |R|                 Stream Identifier (31)                      |
    +=+=============================================================+
    |                   Frame Payload (0...)                      ...
    +---------------------------------------------------------------+
*/
class http2_frame {
public:
    friend http2_frame* http2_recv_frame(int fd);

    virtual ~http2_frame();

    uint32_t get_length() { return length; }
    HTTP2_FRAME_TYPE get_type() { return type; }
    uint8_t get_flags() { return flags; }
    uint32_t get_stream_id() { return stream_id; }
    bool get_reserved() { return reserved; }

    Buffer* get_frame_stream();
    virtual Buffer* get_frame_payload_stream() = 0;

    void set_flags(uint8_t f) { flags = flags | f; }
    void clear_flags(uint8_t f) { flags = flags & ~f; }
    bool has_flags(uint8_t f) { return ((flags & f) == f); }

    void set_stream_id(uint32_t id) { stream_id = id; }

    int send_frame(const int fd);

protected:
    virtual bool parse_frame_payload(const char* buff, const int len) = 0;

    uint32_t length = 0;
    HTTP2_FRAME_TYPE type = HTTP2_SETTINGS_FRAME;
    uint8_t flags = 0;
    uint32_t stream_id = 0;
    bool reserved = false;
};

/*
    ### DATA FRAME ###

    DATA frames (type=0x0) convey arbitrary, variable-length sequences of
    octets associated with a stream.  One or more DATA frames are used,
    for instance, to carry HTTP request or response payloads.

    DATA frames MAY also contain padding.  Padding can be added to DATA
    frames to obscure the size of messages.  Padding is a security feature.

    +---------------+
    |Pad Length? (8)|
    +---------------+-----------------------------------------------+
    |                            Data (*)                         ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/
class http2_data_frame final : public http2_frame {
public:
    http2_data_frame();
    http2_data_frame(Buffer& data, int pad_length);

    uint8_t get_pad_length() { return pad_length; }
    Buffer& get_data() { return data; }

    void set_pad_length(uint8_t len) {
        pad_length = len;
        if(len > 0) set_padded_flag();
        else clear_padded_flag();
    }
    void set_data(Buffer& data);

    bool has_end_stream_flag() { return has_flags(HTTP2_FLAG_END_STREAM); }
    bool has_padded_flag() { return has_flags(HTTP2_FLAG_PADDED); }

    void set_end_stream_flag() { set_flags(HTTP2_FLAG_END_STREAM); }
    void set_padded_flag() { set_flags(HTTP2_FLAG_PADDED); }

    void clear_end_stream_flag() { clear_flags(HTTP2_FLAG_END_HEADERS); }
    void clear_padded_flag() { clear_flags(HTTP2_FLAG_PADDED); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    uint8_t pad_length = 0;
    Buffer data;
};

/*
    ### HEADERS FRAME ###

    The HEADERS frame (type=0x1) is used to open a stream,
    and additionally carries a header block fragment.  HEADERS frames can
    be sent on a stream in the "idle", "reserved (local)", "open", or
    "half-closed (remote)" state.

    +---------------+
    |Pad Length? (8)|
    +-+-------------+-----------------------------------------------+
    |E|                 Stream Dependency? (31)                     |
    +-+-------------+-----------------------------------------------+
    |  Weight? (8)  |
    +-+-------------+-----------------------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/
class http2_headers_frame final : public http2_frame {
public:
    http2_headers_frame();

    uint8_t get_pad_length() { return pad_length; }
    bool get_exclusive() { return exclusive; }
    uint32_t get_stream_dependency() { return stream_dependency; }
    uint8_t get_weight() { return weight; }
    Buffer& get_header() { return header; }

    void set_pad_length(uint8_t len) {
        pad_length = len;
        if(len > 0) set_padded_flag();
        else clear_padded_flag();
    }
    void set_exclusive(bool e) { exclusive = e; }
    void set_stream_dependency(uint32_t dep) { stream_dependency = dep; }
    void set_weight(uint8_t w) { weight = w; }
    // void set_header(char* h) { header = h; }

    bool has_end_stream_flag() { return has_flags(HTTP2_FLAG_END_STREAM); }
    bool has_end_headers_flag() { return has_flags(HTTP2_FLAG_END_HEADERS); }
    bool has_padded_flag() { return has_flags(HTTP2_FLAG_PADDED); }
    bool has_priority_flag() { return has_flags(HTTP2_FLAG_PRIORITY); }

    void set_end_stream_flag() { set_flags(HTTP2_FLAG_END_STREAM); }
    void set_end_headers_flag() { set_flags(HTTP2_FLAG_END_HEADERS); }
    void set_padded_flag() { set_flags(HTTP2_FLAG_PADDED); }
    void set_priority_flag() { set_flags(HTTP2_FLAG_PRIORITY); }

    void clear_end_stream_flag() { clear_flags(HTTP2_FLAG_END_STREAM); }
    void clear_end_headers_flag() { clear_flags(HTTP2_FLAG_END_HEADERS); }
    void clear_padded_flag() { clear_flags(HTTP2_FLAG_PADDED); }
    void clear_priority_flag() { clear_flags(HTTP2_FLAG_PRIORITY); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    uint8_t pad_length = 0;
    bool exclusive = false;
    uint32_t stream_dependency = 0;
    uint8_t weight = 0;
    Buffer header;
};

/*
    ### PRIORITY FRAME ###

    The PRIORITY frame (type=0x2) specifies the sender-advised priority
    of a stream.  It can be sent in any stream state, including idle or 
    closed streams.

    +-+-------------------------------------------------------------+
    |E|                  Stream Dependency (31)                     |
    +-+-------------+-----------------------------------------------+
    |   Weight (8)  |
    +-+-------------+
*/
class http2_priority_frame final : public http2_frame {
public:
    http2_priority_frame();

    bool get_exclusive() { return exclusive; }
    uint32_t get_stream_dependency() { return stream_dependency; }
    uint8_t get_weight() { return weight; }

    void set_exclusive(bool e) { exclusive = e; }
    void set_stream_dependency(uint32_t dep) { stream_dependency = dep; }
    void set_weight(uint8_t w) { weight = w; }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    bool exclusive = false;
    uint32_t stream_dependency = 0;
    uint8_t weight = 0;
    Buffer header;
};

/*
    ### RST_STREAM FRAME ###

    The RST_STREAM frame (type=0x3) allows for immediate termination of a
    stream.  RST_STREAM is sent to request cancellation of a stream or to
    indicate that an error condition has occurred.

    +---------------------------------------------------------------+
    |                        Error Code (32)                        |
    +---------------------------------------------------------------+
*/
class http2_rst_stream_frame final : public http2_frame {
public:
    http2_rst_stream_frame();

    bool get_error_code() { return error_code; }

    void set_error_code(uint32_t e) { error_code = e; }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    uint32_t error_code = 0;
};

/*
    ### SETTINGS FRAME ###

    The SETTINGS frame (type=0x4) conveys configuration parameters that
    affect how endpoints communicate, such as preferences and constraints
    on peer behavior.  The SETTINGS frame is also used to acknowledge the
    receipt of those parameters.  Individually, a SETTINGS parameter can
    also be referred to as a "setting".

    +-------------------------------+
    |       Identifier (16)         |
    +-------------------------------+-------------------------------+
    |                        Value (32)                             |
    +---------------------------------------------------------------+
*/
class http2_settings_frame final : public http2_frame {
public:
    typedef enum _SETTINGS_PARAMETERS {
        SETTINGS_HEADER_TABLE_SIZE = 0x1,
        SETTINGS_ENABLE_PUSH,
        SETTINGS_MAX_CONCURRENT_STREAMS,
        SETTINGS_INITIAL_WINDOW_SIZE,
        SETTINGS_MAX_FRAME_SIZE,
        SETTINGS_MAX_HEADER_LIST_SIZE,
    } SETTINGS_PARAMETERS;

    http2_settings_frame();
    http2_settings_frame(http2_settings set);

    http2_settings get_settings() { return settings; }

    void set_settings(http2_settings set);

    bool has_ack_flag() { return has_flags(HTTP2_FLAG_ACK); }
    void set_ack_flag() { set_flags(HTTP2_FLAG_ACK); }
    void clear_ack_flag() { clear_flags(HTTP2_FLAG_ACK); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    http2_settings settings;
};

/*
    ### PUSH_PROMISE FRAME ###

    The PUSH_PROMISE frame (type=0x5) is used to notify the peer endpoint
    in advance of streams the sender intends to initiate.  The PUSH_PROMISE
    frame includes the unsigned 31-bit identifier of the stream the endpoint
    plans to create along with a set of headers that provide additional
    context for the stream.

    +---------------+
    |Pad Length? (8)|
    +-+-------------+-----------------------------------------------+
    |R|                  Promised Stream ID (31)                    |
    +-+-----------------------------+-------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/
class http2_push_promise_frame final : public http2_frame {
public:
    http2_push_promise_frame();

    uint8_t get_pad_length() { return pad_length; }
    bool get_reserved() { return reserved; }
    uint32_t get_promised_stream_id() { return promised_stream_id; }
    Buffer& get_header_block_fragment() { return header_block_fragment; }

    void set_pad_length(uint8_t len) {
        pad_length = len;
        if(len > 0) set_padded_flag();
        else clear_padded_flag();
    }
    void set_reserved(bool r) { reserved = r; }
    void set_promised_stream_id(uint32_t id) { promised_stream_id = id; }

    bool has_end_headers_flag() { return has_flags(HTTP2_FLAG_END_HEADERS); }
    bool has_padded_flag() { return has_flags(HTTP2_FLAG_PADDED); }

    void set_end_headers_flag() { set_flags(HTTP2_FLAG_END_HEADERS); }
    void set_padded_flag() { set_flags(HTTP2_FLAG_PADDED); }
    
    void clear_end_headers_flag() { clear_flags(HTTP2_FLAG_END_HEADERS); }
    void clear_padded_flag() { clear_flags(HTTP2_FLAG_PADDED); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    uint8_t pad_length = 0;
    bool reserved = false;
    uint32_t promised_stream_id;
    Buffer header_block_fragment;
};

/*
    ### PING FRAME ###

    The PING frame (type=0x6) is a mechanism for measuring a minimal
    round-trip time from the sender, as well as determining whether an
    idle connection is still functional.  PING frames can be sent from
    any endpoint.

    +---------------------------------------------------------------+
    |                                                               |
    |                      Opaque Data (64)                         |
    |                                                               |
    +---------------------------------------------------------------+
*/
class http2_ping_frame final : public http2_frame {
public:
    http2_ping_frame();

    uint64_t get_opaque_data() { return opaque_data; }

    void set_opaque_data(uint64_t data) { opaque_data = data; }

    bool has_ack_flag() { return has_flags(HTTP2_FLAG_ACK); }
    void set_ack_flag() { set_flags(HTTP2_FLAG_ACK); }
    void clear_ack_flag() { clear_flags(HTTP2_FLAG_ACK); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    uint64_t opaque_data = 0;
};

/*
    ### GOAWAY FRAME ###

    The GOAWAY frame (type=0x7) is used to initiate shutdown of a
    connection or to signal serious error conditions.  GOAWAY allows an
    endpoint to gracefully stop accepting new streams while still
    finishing processing of previously established streams.  This enables
    administrative actions, like server maintenance.

    +-+-------------------------------------------------------------+
    |R|                  Last-Stream-ID (31)                        |
    +-+-------------------------------------------------------------+
    |                      Error Code (32)                          |
    +---------------------------------------------------------------+
    |                  Additional Debug Data (*)                    |
    +---------------------------------------------------------------+
*/
class http2_goaway_frame final : public http2_frame {
public:
    http2_goaway_frame();

    bool get_reserved() { return reserved; }
    uint32_t get_last_stream_id() { return last_stream_id; }
    uint32_t get_error_code() { return error_code; }
    Buffer& get_additional_opaque_data() { return additional_opaque_data; }

    void set_reserved(bool r) { reserved = r; }
    void set_last_stream_id(uint32_t id) { last_stream_id = id; }
    void set_error_code(uint32_t code) { error_code = code; }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    bool reserved = false;
    uint32_t last_stream_id;
    uint32_t error_code;
    Buffer additional_opaque_data;
};

/*
    ### WINDOW_UPDATE FRAME ###

    The WINDOW_UPDATE frame (type=0x8) is used to implement flow control.

    +-+-------------------------------------------------------------+
    |R|              Window Size Increment (31)                     |
    +-+-------------------------------------------------------------+
*/
class http2_window_update_frame final : public http2_frame {
public:
    http2_window_update_frame();
    http2_window_update_frame(int size);

    bool get_reserved() { return reserved; }
    uint32_t get_window_size_increment() { return window_size_increment; }

    void set_reserved(bool r) { reserved = r; }
    void set_window_size_increment(int size) { window_size_increment = size; }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    bool reserved = false;
    uint32_t window_size_increment;
};

/*
    ### CONTINUATION FRAME ###

    The CONTINUATION frame (type=0x9) is used to continue a sequence of
    header block fragments. Any number of CONTINUATION frames can be sent, 
    as long as the preceding frame is on the same stream and is a HEADERS, 
    PUSH_PROMISE, or CONTINUATION frame without the END_HEADERS flag set.

    +---------------------------------------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
*/
class http2_continuation_frame final : public http2_frame {
public:
    http2_continuation_frame();

    Buffer& get_header_block_fragment() { return header_block_fragment; }

    bool has_end_headers_flag() { return has_flags(HTTP2_FLAG_END_HEADERS); }
    void set_end_headers_flag() { set_flags(HTTP2_FLAG_END_HEADERS); }
    void clear_end_headers_flag() { clear_flags(HTTP2_FLAG_END_HEADERS); }

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;

    Buffer header_block_fragment;
};

void http2_send_preface(int fd);
bool http2_check_preface(int fd);

http2_frame* http2_recv_frame(int fd);

const char* http2_get_frame_type_name(HTTP2_FRAME_TYPE type);

#endif
