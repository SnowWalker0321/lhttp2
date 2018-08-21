#ifndef _HTTP2_FRAME_H_
#define _HTTP2_FRAME_H_

#include <stdint.h>

#include "Buffer.h"
#include "http2_settings.h"
#include "http2_error.h"

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
    friend http2_frame* http2_recv_frame(const int fd);

    virtual ~http2_frame() = 0;

    uint32_t get_length();
    HTTP2_FRAME_TYPE get_type();
    uint8_t get_flags();
    uint32_t get_stream_id();
    bool get_reserved();

    void set_flags(uint8_t flags);
    void clear_flags(uint8_t flags);
    bool has_flags(uint8_t flags);

    void set_stream_id(uint32_t stream_id);

    Buffer* get_frame_stream();
    virtual Buffer* get_frame_payload_stream() = 0;

    int send_frame(const int fd);

protected:
    virtual bool parse_frame_payload(const char* buff, const int len) = 0;
    virtual void update_length() = 0;

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
    http2_data_frame(Buffer data, uint8_t pad_length = 0);
    ~http2_data_frame();

    uint8_t get_pad_length();
    Buffer get_data();

    void set_pad_length(uint8_t pad_length);
    void set_data(Buffer& data);

    bool has_end_stream_flag();
    bool has_padded_flag();

    void set_end_stream_flag();
    void set_padded_flag();

    void clear_end_stream_flag();
    void clear_padded_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_headers_frame(Buffer header, uint8_t pad_length);
    http2_headers_frame(Buffer header, bool exclusive, uint32_t stream_dependency, uint8_t weight, uint8_t pad_length = 0);
    ~http2_headers_frame();

    uint8_t get_pad_length();
    bool get_exclusive();
    uint32_t get_stream_dependency();
    uint8_t get_weight();
    Buffer get_header();

    void set_pad_length(uint8_t pad_length);
    void set_exclusive(bool exclusive);
    void set_stream_dependency(uint32_t stream_dependency);
    void set_weight(uint8_t weight);
    void set_header(Buffer& header);

    bool has_end_stream_flag();
    bool has_end_headers_flag();
    bool has_padded_flag();
    bool has_priority_flag();

    void set_end_stream_flag();
    void set_end_headers_flag();
    void set_padded_flag();
    void set_priority_flag();

    void clear_end_stream_flag();
    void clear_end_headers_flag();
    void clear_padded_flag();
    void clear_priority_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_priority_frame(bool exclusive, uint32_t stream_dependency, uint8_t weight);
    ~http2_priority_frame();

    bool get_exclusive();
    uint32_t get_stream_dependency();
    uint8_t get_weight();

    void set_exclusive(bool exclusive);
    void set_stream_dependency(uint32_t stream_dependency);
    void set_weight(uint8_t weight);

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_rst_stream_frame(uint32_t error_code);
    ~http2_rst_stream_frame();

    bool get_error_code();

    void set_error_code(uint32_t error_code);

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    ~http2_settings_frame();

    http2_settings get_settings();

    void set_settings(http2_settings set);

    bool has_ack_flag();
    void set_ack_flag();
    void clear_ack_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_push_promise_frame(uint32_t promised_stream_id, Buffer header_block_fragment, uint8_t pad_length = 0);
    ~http2_push_promise_frame();

    uint8_t get_pad_length();
    bool get_reserved();
    uint32_t get_promised_stream_id();
    Buffer get_header_block_fragment();

    void set_pad_length(uint8_t pad_length);
    void set_reserved(bool reserved);
    void set_promised_stream_id(uint32_t promised_stream_id);
    void set_header_block_fragment(Buffer header_block_fragment);

    bool has_end_headers_flag();
    bool has_padded_flag();

    void set_end_headers_flag();
    void set_padded_flag();
    
    void clear_end_headers_flag();
    void clear_padded_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_ping_frame(uint64_t opaque_data);
    ~http2_ping_frame();

    uint64_t get_opaque_data();
    void set_opaque_data(uint64_t data);

    bool has_ack_flag();
    void set_ack_flag();
    void clear_ack_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_goaway_frame(uint32_t last_stream_id, uint32_t error_code, Buffer additional_debug_data);
    ~http2_goaway_frame();

    bool get_reserved();
    uint32_t get_last_stream_id();
    uint32_t get_error_code();
    Buffer get_additional_debug_data();

    void set_reserved(bool reserved);
    void set_last_stream_id(uint32_t last_stream_id);
    void set_error_code(uint32_t error_code);
    void set_additional_debug_data(Buffer additional_debug_data);

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

    bool reserved = false;
    uint32_t last_stream_id;
    uint32_t error_code;
    Buffer additional_debug_data;
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
    http2_window_update_frame(uint32_t size);
    ~http2_window_update_frame();

    bool get_reserved();
    uint32_t get_window_size_increment();

    void set_reserved(bool reserved);
    void set_window_size_increment(int window_size_increment);

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

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
    http2_continuation_frame(Buffer header_block_fragment);
    ~http2_continuation_frame();

    Buffer get_header_block_fragment();
    void set_header_block_fragment(Buffer header_block_fragment);

    bool has_end_headers_flag();
    void set_end_headers_flag();
    void clear_end_headers_flag();

    Buffer* get_frame_payload_stream() override;

private:
    bool parse_frame_payload(const char* buff, const int len) override;
    void update_length() override;

    Buffer header_block_fragment;
};

http2_frame* http2_recv_frame(const int fd);
int http2_send_frame(http2_frame* frame, const int fd);

const char* http2_get_frame_type_name(HTTP2_FRAME_TYPE type);

#endif
