#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include "frame.h"

using namespace lhttp2;

/*
    Implementation of frame header
*/
Frame::~Frame() {
}

const uint32_t Frame::length() const {
    return length_;
}

const Frame::FRAME_TYPE Frame::type() const {
    return type_;
}

const uint8_t Frame::flags() const {
    return flags_;
}

const uint32_t Frame::stream_id() const {
    return stream_id_;
}

const bool Frame::reserved() const {
    return reserved_;
}

void Frame::set_flags(uint8_t flags) {
    flags_ = flags_ | flags;
}

void Frame::clear_flags(uint8_t flags) {
    flags_ = flags_ & ~flags;
}

bool Frame::has_flags(uint8_t flags) const {
    return ((flags_ & flags) == flags);
}

void Frame::set_stream_id(uint32_t streamId) {
    stream_id_ = streamId;
}

Frame* Frame::RecvFrame(const int fd, hpack::Table& hpack_table, bool debug) {
    if(fd < 0) {
        return nullptr;
    }

    Frame *frame;
    char header_buff[9];
    uint8_t flags;
    uint32_t length, stream_id;
    bool reserved;
    FRAME_TYPE type;

    if(::read(fd, header_buff, 9) != 9) {
        return nullptr;
    }

    length = (uint32_t)header_buff[0] << 16 | \
             (uint32_t)header_buff[1] << 8 | \
             (uint32_t)header_buff[2];

    type = (FRAME_TYPE)header_buff[3];
    flags = (uint8_t)header_buff[4];
    reserved = ((header_buff[5] & 0x80) == 0x80);

    stream_id = (uint32_t)(header_buff[5] & 0x7F) << 24 | \
                (uint32_t)header_buff[6] << 16 | \
                (uint32_t)header_buff[7] << 8 | \
                (uint32_t)header_buff[8];

    if(type > 0x09) {
        return nullptr;
    }

    char* payload_buff = new char[length];

    if(::read(fd, payload_buff, length) != length) {
        delete[] payload_buff;
        return nullptr;
    }

    if(type == TYPE_DATA_FRAME)
        frame = new DataFrame();
    else if(type == TYPE_HEADERS_FRAME)
        frame = new HeadersFrame();
    else if(type == TYPE_PRIORITY_FRAME)
        frame = new PriorityFrame();
    else if(type == TYPE_RST_STREAM_FRAME)
        frame = new RSTStreamFrame();
    else if(type == TYPE_SETTINGS_FRAME)
        frame = new SettingsFrame();
    else if(type == TYPE_PUSH_PROMISE_FRAME)
        frame = new PushPromisFrame();
    else if(type == TYPE_PING_FRAME)
        frame = new PingFrame();
    else if(type == TYPE_GOAWAY_FRAME)
        frame = new GoawayFrame();
    else if(type == TYPE_WINDOW_UPDATE_FRAME)
        frame = new WindowUpdateFrame();
    else if(type == TYPE_CONTINUATION_FRAME)
        frame = new ContinuationFrame();
    else {
        delete[] payload_buff;
        return nullptr;
    }

    frame->length_ = length;
    frame->type_ = type;
    frame->flags_ = flags;
    frame->reserved_ = reserved;
    frame->stream_id_ = stream_id;

    if(debug) {
        Buffer::PrintBuffer(header_buff, 9);
        Buffer::PrintBuffer(payload_buff, length);
    }

    frame->DecodeFramePayload((const char *)payload_buff, frame->length_, hpack_table);

    delete[] payload_buff;

    return frame;
}

int Frame::SendFrame(const int fd, Frame* frame, hpack::Table& hpack_table, bool debug) {
    Buffer *stream = frame->EncodeFrame(hpack_table);
    if(debug == true) stream->Print();

    signal(SIGPIPE, SIG_IGN);
    int len = ::send(fd, stream->Address(), stream->Length(), 0);

    delete stream;
    return len;
}

const std::string Frame::GetFrameTypeName(FRAME_TYPE type) {
    switch(type) {
        case TYPE_DATA_FRAME: return "DATA";
        case TYPE_HEADERS_FRAME: return "HEADERS";
        case TYPE_PRIORITY_FRAME: return "PRIORITY";
        case TYPE_RST_STREAM_FRAME: return "RST_STREAM";
        case TYPE_SETTINGS_FRAME: return "SETTINGS";
        case TYPE_PUSH_PROMISE_FRAME: return "PUSH_PROMISE";
        case TYPE_PING_FRAME: return "PING";
        case TYPE_GOAWAY_FRAME: return "GOAWAY";
        case TYPE_WINDOW_UPDATE_FRAME: return "WINDOW_UPDATE";
        case TYPE_CONTINUATION_FRAME: return "CONTINUATION";
        default: break;
    }
    return "UNKNOWN";
}

Buffer* Frame::EncodeFrame(hpack::Table& hpack_table) {
    Buffer* payloadBuffer = EncodeFramePayload(hpack_table);
    Buffer* headerBuffer = new Buffer(9 + payloadBuffer->Length());

    headerBuffer->SetValue(payloadBuffer->Length(), 3, 0);
    headerBuffer->SetValue(type_, 1, 3);
    headerBuffer->SetValue(flags_, 1, 4);
    headerBuffer->SetValue((reserved_ << 7) | (stream_id_ & 0x7FFFFFFF), 4, 5);

    headerBuffer->Append(*payloadBuffer);
    delete payloadBuffer;

    return headerBuffer;
}

/*
    Implementation of DATA FRAME
*/
DataFrame::DataFrame() {
    type_ = TYPE_DATA_FRAME;
}

DataFrame::DataFrame(Buffer data, uint8_t pad_length) {
    DataFrame();

    pad_length_ = pad_length;
    if(pad_length_ > 0) set_flags(FLAG_PADDED);
    else clear_flags(FLAG_PADDED);

    data_ = data;

    UpdateLength();
}

DataFrame::~DataFrame() {
}

const uint8_t DataFrame::pad_length() const {
    return pad_length_;
}

const Buffer& DataFrame::data() const {
    return data_;
}

void DataFrame::set_pad_length(uint8_t pad_length) {
    pad_length_ = pad_length;
}

void DataFrame::set_data(Buffer& data) {
    data_ = data;
    UpdateLength();
}

bool DataFrame::has_end_stream_flag() const {
    return has_flags(FLAG_END_STREAM);
}

bool DataFrame::has_padded_flag() const {
    return has_flags(FLAG_PADDED);
}

void DataFrame::set_end_stream_flag() {
    set_flags(FLAG_END_STREAM);
}

void DataFrame::set_padded_flag() {
    set_flags(FLAG_PADDED);
    UpdateLength();
}

void DataFrame::clear_end_stream_flag() {
    clear_flags(FLAG_END_HEADERS);
}

void DataFrame::clear_padded_flag() {
    clear_flags(FLAG_PADDED);
    UpdateLength();
}

Buffer* DataFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(length_);

    if(has_padded_flag()) {
        stream->Set(pad_length_, 0);
        stream->Append(data_);
    }
    else {
        stream->Append(data_);
    }

    return stream;
}

bool DataFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length_ = buff[idx];
        idx = idx + 1;
    }

    data_ = Buffer(buff + idx, len - pad_length_ - idx);
    UpdateLength();

    return true;
}

void DataFrame::UpdateLength() {
    length_ = data_.Length();

    if(has_padded_flag())
        length_ = length_ + pad_length_ + 1;
}

/*
    Implementation of HEADERS FRAME
*/
HeadersFrame::HeadersFrame() {
    type_ = TYPE_HEADERS_FRAME;
}

HeadersFrame::HeadersFrame(std::vector<hpack::HeaderFieldRepresentation> header_list, hpack::Table& hpack_table, uint8_t pad_length) {
    HeadersFrame();

    pad_length_ = pad_length;
    if(pad_length_ > 0) set_flags(FLAG_PADDED);
    else clear_flags(FLAG_PADDED);

    clear_flags(FLAG_PRIORITY);

    header_list_ = header_list;
    update_header_block_fragment(hpack_table);
}

HeadersFrame::HeadersFrame(std::vector<hpack::HeaderFieldRepresentation> header_list, hpack::Table& hpack_table, bool exclusive, uint32_t stream_dependency, uint8_t weight, uint8_t pad_length) {
    HeadersFrame();

    pad_length_ = pad_length;
    if(pad_length_ > 0) set_flags(FLAG_PADDED);
    else clear_flags(FLAG_PADDED);

    set_flags(FLAG_PRIORITY);
    exclusive_ = exclusive;
    stream_dependency_ = stream_dependency;
    weight_ = weight;

    header_list_ = header_list;
    update_header_block_fragment(hpack_table);
}

HeadersFrame::~HeadersFrame() {
}

const uint8_t HeadersFrame::pad_length() const {
    return pad_length_;
}

const bool HeadersFrame::exclusive() const { 
    return exclusive_;
}

const uint32_t HeadersFrame::stream_dependency() const {
    return stream_dependency_;
}

const uint8_t HeadersFrame::weight() const {
    return weight_;
}

const std::vector<hpack::HeaderFieldRepresentation>& HeadersFrame::header_list() const {
    return header_list_;
}

const Buffer& HeadersFrame::header_block_fragment() const {
    return header_;
}

void HeadersFrame::set_pad_length(uint8_t pad_length) {
    pad_length_ = pad_length;
}

void HeadersFrame::set_exclusive(bool exclusive) {
    exclusive_ = exclusive;
}

void HeadersFrame::set_stream_dependency(uint32_t stream_dependency) {
    stream_dependency_ = stream_dependency;
}

void HeadersFrame::set_weight(uint8_t weight) {
    weight_ = weight;
}

void HeadersFrame::set_header_list(std::vector<hpack::HeaderFieldRepresentation> headerList, hpack::Table& hpack_table) {
    header_list_ = headerList;
    update_header_block_fragment(hpack_table);
}

void HeadersFrame::update_header_block_fragment(hpack::Table& hpack_table) {
    hpack_table.Encode(header_, header_list_, false);
    UpdateLength();
}

bool HeadersFrame::has_end_stream_flag() const {
    return has_flags(FLAG_END_STREAM);
}

bool HeadersFrame::has_end_headers_flag() const {
    return has_flags(FLAG_END_HEADERS);
}

bool HeadersFrame::has_padded_flag() const {
    return has_flags(FLAG_PADDED);
}

bool HeadersFrame::has_priority_flag() const {
    return has_flags(FLAG_PRIORITY);
}

void HeadersFrame::set_end_stream_flag() {
    set_flags(FLAG_END_STREAM);
}

void HeadersFrame::set_end_headers_flag() {
    set_flags(FLAG_END_HEADERS);
}

void HeadersFrame::set_padded_flag() {
    set_flags(FLAG_PADDED);
    UpdateLength();
}

void HeadersFrame::set_priority_flag() {
    set_flags(FLAG_PRIORITY);
    UpdateLength();
}

void HeadersFrame::clear_end_stream_flag() {
    clear_flags(FLAG_END_STREAM);
}

void HeadersFrame::clear_end_headers_flag() {
    clear_flags(FLAG_END_HEADERS);
}

void HeadersFrame::clear_padded_flag() {
    clear_flags(FLAG_PADDED);
    UpdateLength();
}

void HeadersFrame::clear_priority_flag() {
    clear_flags(FLAG_PRIORITY);
    UpdateLength();
}

Buffer* HeadersFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    update_header_block_fragment(hpack_table);
    int idx = 0;
    Buffer *stream = new Buffer(length_);

    if(has_padded_flag()) {
        stream->Set(pad_length_, idx);
        idx = idx + 1;
    }

    if(has_priority_flag()) {
        stream->SetValue((exclusive_ << 7) | (stream_dependency_ & 0x7FFFFFFF), 4, idx);
        stream->Set(weight_, idx + 4);
        idx = idx + 5;
    }

    stream->Append(header_);

    return stream;
}

bool HeadersFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length_ = buff[idx];
        idx = idx + 1;
    }

    if(has_priority_flag()) {
        exclusive_ = ((buff[idx] & 0x80) == 0x80);
        stream_dependency_ = (uint32_t)(buff[idx] & 0x7F) << 24 | \
                            (uint32_t)buff[idx + 1] << 16 | \
                            (uint32_t)buff[idx + 2] << 8 | \
                            (uint32_t)buff[idx + 3];
        weight_ = buff[idx + 4];
        idx = idx + 5;
    }

    header_ = Buffer(buff + idx, len - pad_length_ - idx);
    if(hpack_table.Decode(header_list_, header_) == false) {
        return false;
    }

    UpdateLength();

    return true;
}

void HeadersFrame::UpdateLength() {
    length_ = header_.Length();

    if(has_padded_flag())
        length_ = length_ + pad_length_ + 1;

    if(has_priority_flag())
        length_ = length_ + 5;
}

/*
    Implementation of PRIORITY FRAME
*/
PriorityFrame::PriorityFrame() {
    type_ = TYPE_PRIORITY_FRAME;
    length_ = 5;
}

PriorityFrame::PriorityFrame(bool exclusive, uint32_t stream_dependency, uint8_t weight) {
    PriorityFrame();
    exclusive_ = exclusive;
    stream_dependency_ = stream_dependency;
    weight_ = weight;
}

PriorityFrame::~PriorityFrame() {
}

const bool PriorityFrame::exclusive() const {
    return exclusive_;
}

const uint32_t PriorityFrame::stream_dependency() const {
    return stream_dependency_;
}

const uint8_t PriorityFrame::weight() const {
    return weight_;
}

void PriorityFrame::set_exclusive(bool exclusive) {
    exclusive_ = exclusive;
}

void PriorityFrame::set_stream_dependency(uint32_t stream_dependency) {
    stream_dependency_ = stream_dependency;
}

void PriorityFrame::set_weight(uint8_t weight) {
    weight_ = weight;
}

Buffer* PriorityFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(40);

    stream->SetValue((exclusive_ << 7) | (stream_dependency_ & 0x7FFFFFFF), 4, 0);
    stream->Set(weight_, 4);

    return stream;
}

bool PriorityFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    if(len != 5) return false;

    exclusive_ = ((buff[0] & 0x80) == 0x80);
    stream_dependency_ = (uint32_t)(buff[0] & 0x7F) << 24 | \
                        (uint32_t)buff[1] << 16 | \
                        (uint32_t)buff[2] << 8 | \
                        (uint32_t)buff[3];
    weight_ = buff[4];

    UpdateLength();

    return true;
}

void PriorityFrame::UpdateLength() {
    length_ = 5;
}

/*
    Implementation of RST_STREAM FRAME
*/
RSTStreamFrame::RSTStreamFrame() {
    type_ = TYPE_RST_STREAM_FRAME;
    length_ = 4;
}

RSTStreamFrame::RSTStreamFrame(uint32_t error_code) {
    RSTStreamFrame();
    error_code_ = error_code;
}

RSTStreamFrame::~RSTStreamFrame() {
}

const uint32_t RSTStreamFrame::error_code() const {
    return error_code_;
}

void RSTStreamFrame::set_error_code(uint32_t error_code) {
    error_code_ = error_code;
}

Buffer* RSTStreamFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(32);

    stream->SetValue(error_code_, 4, 0);

    UpdateLength();

    return stream;
}

bool RSTStreamFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    if(len != 4) return false;

    error_code_ = (uint32_t)buff[0] << 24 | \
                 (uint32_t)buff[1] << 16 | \
                 (uint32_t)buff[2] << 8 | \
                 (uint32_t)buff[3];

    return true;
}

void RSTStreamFrame::UpdateLength() {
    length_ = 4;
}

/*
    Implementation of SETTINGS FRAME
*/
SettingsFrame::SettingsFrame() {
    type_ = TYPE_SETTINGS_FRAME;
}

SettingsFrame::SettingsFrame(lhttp2::Settings settings) {
    SettingsFrame();
    settings_ = settings;
    UpdateLength();
}

SettingsFrame::~SettingsFrame() {
}

const lhttp2::Settings& SettingsFrame::settings() const {
    return settings_;
}

void SettingsFrame::set_settings(lhttp2::Settings& settings) {
    settings_ = settings;
    UpdateLength();
}

bool SettingsFrame::has_ack_flag() {
    return has_flags(FLAG_ACK);
}

void SettingsFrame::set_ack_flag() {
    set_flags(FLAG_ACK);
    UpdateLength();
}

void SettingsFrame::clear_ack_flag() {
    clear_flags(FLAG_ACK);
    UpdateLength();
}

Buffer* SettingsFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    if(has_ack_flag() == true || length_ == 0) {
        return new Buffer((unsigned int)0);
    }

    int idx = 0;
    Buffer *stream = new Buffer(length_);

    if(settings_.header_table_size() != 0x1000) {
        stream->SetValue(0x0001, 2, idx);
        stream->SetValue(settings_.header_table_size(), 4, idx + 2);
        idx = idx + 6;
    }

    if(settings_.enable_push() != true) {
        stream->SetValue(0x0002, 2, idx);
        stream->SetValue(settings_.enable_push(), 4, idx + 2);
        idx = idx + 6;
    }

    if(settings_.max_concurrent_stream() != UINT32_MAX) {
        stream->SetValue(0x0003, 2, idx);
        stream->SetValue(settings_.max_concurrent_stream(), 4, idx + 2);
        idx = idx + 6;
    }

    if(settings_.initial_window_size() != 0xFFFF) {
        stream->SetValue(0x0004, 2, idx);
        stream->SetValue(settings_.initial_window_size(), 4, idx + 2);
        idx = idx + 6;
    }

    if(settings_.max_frame_size() != 0x4000) {
        stream->SetValue(0x0005, 2, idx);
        stream->SetValue(settings_.max_frame_size(), 4, idx + 2);
        idx = idx + 6;
    }

    if(settings_.max_header_list_size() != UINT32_MAX) {
        stream->SetValue(0x0006, 2, idx);
        stream->SetValue(settings_.max_header_list_size(), 4, idx + 2);
        idx = idx + 6;
    }
    
    return stream;
}

bool SettingsFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    if(len % 6 != 0) return false;

    int i, set_cnt = len / 6;
    uint32_t id, val;
    lhttp2::Settings settings;

    for(i = 0; i < set_cnt; i++) {
        id = (uint32_t)buff[i * 6] << 8 | \
             (uint32_t)buff[i * 6 + 1];

        val = (uint32_t)buff[i * 6 + 2] << 24 | \
              (uint32_t)buff[i * 6 + 3] << 16 | \
              (uint32_t)buff[i * 6 + 4] << 8 | \
              (uint32_t)buff[i * 6 + 5];

        if(id == SETTINGS_HEADER_TABLE_SIZE) settings.set_header_table_size(val);
        else if(id == SETTINGS_ENABLE_PUSH) settings.set_enable_push(val);
        else if(id == SETTINGS_MAX_CONCURRENT_STREAMS) settings.set_max_concurrent_stream(val);
        else if(id == SETTINGS_INITIAL_WINDOW_SIZE) settings.set_initial_window_size(val);
        else if(id == SETTINGS_MAX_FRAME_SIZE) {
            if(val < 0x4000) val = 0x4000;
            if(val > 0xFFFFFF) val = 0xFFFFFF;
            settings.set_max_frame_size(val);
        }
        else if(id == SETTINGS_MAX_HEADER_LIST_SIZE) settings.set_max_header_list_size(val);
    }

    settings_ = settings;
    UpdateLength();

    return true;
}

void SettingsFrame::UpdateLength() {
    if(has_ack_flag()) {
        length_ = 0;
        return;
    }

    int setCount = 0;

    if(settings_.header_table_size() != 0x1000) setCount++;
    if(settings_.enable_push() != true) setCount++;
    if(settings_.max_concurrent_stream() != UINT32_MAX) setCount++;
    if(settings_.initial_window_size() != 0xFFFF) setCount++;
    if(settings_.max_frame_size() != 0x4000) setCount++;
    if(settings_.max_header_list_size() != UINT32_MAX) setCount++;

    length_ = setCount * 6;
}

/*
    Implementation of PUSH_PROMISE FRAME
*/
PushPromisFrame::PushPromisFrame() {
    type_ = TYPE_PUSH_PROMISE_FRAME;
    length_ = 4;
}

PushPromisFrame::PushPromisFrame(uint32_t promised_stream_id, Buffer header_block_fragment, uint8_t pad_length) {
    PushPromisFrame();

    promised_stream_id_ = promised_stream_id;
    header_block_fragment_ = header_block_fragment;

    pad_length_ = pad_length;
    if(pad_length_ > 0) set_flags(FLAG_PADDED);
    else clear_flags(FLAG_PADDED);

    UpdateLength();
}

PushPromisFrame::~PushPromisFrame() {
}

const uint8_t PushPromisFrame::pad_length() const {
    return pad_length_;
}

const bool PushPromisFrame::reserved() const {
    return reserved_;
}

const uint32_t PushPromisFrame::promised_stream_id() const {
    return promised_stream_id_;
}

const Buffer& PushPromisFrame::header_block_fragment() const {
    return header_block_fragment_;
}

void PushPromisFrame::set_pad_length(uint8_t pad_length) {
    pad_length_ = pad_length;
}

void PushPromisFrame::set_reserved(bool reserved) {
    reserved_ = reserved;
}

void PushPromisFrame::set_promised_stream_id(uint32_t promised_stream_id) {
    promised_stream_id_ = promised_stream_id;
}

void PushPromisFrame::set_header_block_fragment(Buffer& header_block_fragment) {
    header_block_fragment_ = header_block_fragment;
    UpdateLength();
}

bool PushPromisFrame::has_end_headers_flag() {
    return has_flags(FLAG_END_HEADERS);
}

bool PushPromisFrame::has_padded_flag() {
    return has_flags(FLAG_PADDED);
}

void PushPromisFrame::set_end_headers_flag() {
    set_flags(FLAG_END_HEADERS);
}

void PushPromisFrame::set_padded_flag() {
    set_flags(FLAG_PADDED);
    UpdateLength();
}

void PushPromisFrame::clear_end_headers_flag() {
    clear_flags(FLAG_END_HEADERS);
}

void PushPromisFrame::clear_padded_flag() {
    clear_flags(FLAG_PADDED);
    UpdateLength();
}

Buffer* PushPromisFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    int idx = 0;
    Buffer *stream = new Buffer(pad_length_ + header_block_fragment_.Length());

    if(has_padded_flag()) {
        stream->Set(pad_length_, 0);
        idx = 1;
    }

    stream->SetValue((reserved_ << 7) | (promised_stream_id_ & 0x7FFFFFFF), 4, idx);
    stream->Append(header_block_fragment_);

    return stream;
}

bool PushPromisFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    int idx = 0;

    if(has_padded_flag()) {
        pad_length_ = buff[idx];
        idx = idx + 1;
    }

    reserved_ = ((buff[idx] & 0x80) == 0x80);
    promised_stream_id_ = (uint32_t)(buff[idx] & 0x7F) << 24 | \
                        (uint32_t)buff[idx + 1] << 16 | \
                        (uint32_t)buff[idx + 2] << 8 | \
                        (uint32_t)buff[idx + 3];
    
    header_block_fragment_ = Buffer(buff + idx + 4, len - pad_length_ - idx);
    UpdateLength();

    return true;
}

void PushPromisFrame::UpdateLength() {
    length_ = 4 + header_block_fragment_.Length();

    if(has_padded_flag())
        length_ = length_ + pad_length_ + 1;
}

/*
    Implementation of PING FRAME
*/
PingFrame::PingFrame() {
    type_ = TYPE_PING_FRAME;
    length_ = 8;
}

PingFrame::PingFrame(uint64_t opaque_data) {
    PingFrame();
    opaque_data_ = opaque_data;
}

PingFrame::~PingFrame() {
}

const uint64_t PingFrame::opaque_data() const {
    return opaque_data_;
}

void PingFrame::set_opaque_data(uint64_t opaque_data) {
    opaque_data_ = opaque_data;
}

bool PingFrame::has_ack_flag() {
    return has_flags(FLAG_ACK);
}

void PingFrame::set_ack_flag() {
    set_flags(FLAG_ACK);
}

void PingFrame::clear_ack_flag() {
    clear_flags(FLAG_ACK);
}

Buffer* PingFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(8);

    stream->SetValue(opaque_data_, 8, 0);

    return stream;
}

bool PingFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    opaque_data_ = (uint64_t)buff[0] << 56 | \
                  (uint64_t)buff[1] << 48 | \
                  (uint64_t)buff[2] << 40 | \
                  (uint64_t)buff[3] << 32 | \
                  (uint64_t)buff[4] << 24 | \
                  (uint64_t)buff[5] << 16 | \
                  (uint64_t)buff[6] << 8 | \
                  (uint64_t)buff[7];

    UpdateLength();

    return true;
}

void PingFrame::UpdateLength() {
    length_ = 8;
}

/*
    Implementation of GOAWAY FRAME
*/
GoawayFrame::GoawayFrame() {
    type_ = TYPE_GOAWAY_FRAME;
    length_ = 8;
}

GoawayFrame::GoawayFrame(uint32_t last_stream_id, uint32_t error_code, Buffer additional_debug_data) {
    GoawayFrame();

    last_stream_id_ = last_stream_id;
    error_code_ = error_code;
    additional_debug_data_ = additional_debug_data;

    UpdateLength();
}

GoawayFrame::~GoawayFrame() {
}

const bool GoawayFrame::reserved() const {
    return reserved_;
}

const uint32_t GoawayFrame::last_stream_id() const {
    return last_stream_id_;
}

const uint32_t GoawayFrame::error_code() const {
    return error_code_;
}

const Buffer& GoawayFrame::additional_debug_data() const {
    return additional_debug_data_;
}

void GoawayFrame::set_reserved(bool reserved) {
    reserved_ = reserved;
}

void GoawayFrame::set_last_stream_id(uint32_t last_stream_id) {
    last_stream_id_ = last_stream_id;
}

void GoawayFrame::set_error_code(uint32_t error_code) {
    error_code_ = error_code;
}

void GoawayFrame::set_additional_debug_data(Buffer& additional_debug_data) {
    additional_debug_data_ = additional_debug_data;
    UpdateLength();
}

Buffer* GoawayFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(8 + additional_debug_data_.Length());

    stream->SetValue((reserved_ << 7) | (last_stream_id_ & 0x7FFFFFFF), 4, 0);
    stream->SetValue(error_code_, 4, 4);
    stream->Append(additional_debug_data_);

    return stream;
}

bool GoawayFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    reserved_ = ((buff[0] & 0x80) == 0x80);

    last_stream_id_ = (uint32_t)(buff[0] & 0x7F) << 24 | \
                    (uint32_t)buff[1] << 16 | \
                    (uint32_t)buff[2] << 8 | \
                    (uint32_t)buff[3];

    error_code_ = (uint32_t)(buff[4] & 0x7F) << 24 | \
                (uint32_t)buff[5] << 16 | \
                (uint32_t)buff[6] << 8 | \
                (uint32_t)buff[7];

    additional_debug_data_ = Buffer(buff + 8, len - 8);

    UpdateLength();

    return true;
}

void GoawayFrame::UpdateLength() {
    length_ = 8 + additional_debug_data_.Length();
}

/*
    Implementation of WINDOW_UPDATE FRAME
*/
WindowUpdateFrame::WindowUpdateFrame() {
    type_ = TYPE_WINDOW_UPDATE_FRAME;
    length_ = 4;
}

WindowUpdateFrame::WindowUpdateFrame(uint32_t window_size_increment) {
    WindowUpdateFrame();
    window_size_increment_ = window_size_increment;
}

WindowUpdateFrame::~WindowUpdateFrame() {
}

const bool WindowUpdateFrame::reserved() const {
    return reserved_;
}

const uint32_t WindowUpdateFrame::window_size_increment() const {
    return window_size_increment_;
}

void WindowUpdateFrame::set_reserved(bool reserved) {
    reserved_ = reserved;
}

void WindowUpdateFrame::set_window_size_increment(int window_size_increment) {
    window_size_increment_ = window_size_increment;
}

Buffer* WindowUpdateFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(4);

    stream->SetValue((reserved_ << 7) | (window_size_increment_ & 0x7FFFFFFF), 4, 0);

    return stream;
}

bool WindowUpdateFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    reserved_ = ((buff[0] & 0x80) == 0x80);

    window_size_increment_ = (uint32_t)(buff[0] & 0x7F) << 24 | \
                            (uint32_t)buff[1] << 16 | \
                            (uint32_t)buff[2] << 8 | \
                            (uint32_t)buff[3];

    UpdateLength();

    return true;
}

void WindowUpdateFrame::UpdateLength() {
    length_ = 4;
}

/*
    Implementation of CONTINUATION FRAME
*/
ContinuationFrame::ContinuationFrame() {
    type_ = TYPE_CONTINUATION_FRAME;
}

ContinuationFrame::ContinuationFrame(Buffer& header_block_fragment) {
    ContinuationFrame();
    header_block_fragment_ = header_block_fragment;
    UpdateLength();
}

ContinuationFrame::~ContinuationFrame() {
}

const Buffer& ContinuationFrame::header_block_fragment() const {
    return header_block_fragment_;
}

void ContinuationFrame::set_header_block_fragment(Buffer& header_block_fragment) {
    header_block_fragment_ = header_block_fragment;
    UpdateLength();
}

bool ContinuationFrame::has_end_headers_flag() {
    return has_flags(FLAG_END_HEADERS);
}

void ContinuationFrame::set_end_headers_flag() {
    set_flags(FLAG_END_HEADERS);
}

void ContinuationFrame::clear_end_headers_flag() {
    clear_flags(FLAG_END_HEADERS);
}

Buffer* ContinuationFrame::EncodeFramePayload(hpack::Table& hpack_table) {
    Buffer *stream = new Buffer(header_block_fragment_);
    return stream;
}

bool ContinuationFrame::DecodeFramePayload(const char* buff, const int len, hpack::Table& hpack_table) {
    header_block_fragment_ = Buffer(buff, len);
    UpdateLength();
    return true;
}

void ContinuationFrame::UpdateLength() {
    length_ = header_block_fragment_.Length();
}