#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection.h"

using namespace lhttp2;

// The client Connection preface starts with a sequence of 24 octets,
// "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define PREFACE "\x50\x52\x49\x20\x2a\x20\x48\x54\x54\x50\x2f\x32\x2e\x30\x0d\x0a\x0d\x0a\x53\x4d\x0d\x0a\x0d\x0a"
#define PREFACE_LEN 24

static const char preface[] = PREFACE;

Connection::Connection(int fd, ENDPOINT_TYPE type, lhttp2::Settings settings) : fd_(fd), type_(type), settings_(settings) {
    if(type_ == ENDPOINT_CLIENT) {
        SendPreface();
        SettingsFrame settings_frame;
        settings_frame.SetSettings(settings_);
        Frame::SendFrame(fd, &settings_frame, hpack_table_);
    }
    else if(type_ == ENDPOINT_SERVER) {
        if(RecvPreface() == false) {
            ::close(fd_);
            return;
        }

        Frame* frame = Frame::RecvFrame(fd, hpack_table_);
        if(frame->Type() != Frame::TYPE_SETTINGS_FRAME) {
            ::close(fd_);
            return;
        }

        settings_ = ((SettingsFrame*)frame)->Settings();
        delete frame;

        SettingsFrame settings_frame;
        settings_frame.SetAckFlag();
        Frame::SendFrame(fd_, &settings_frame, hpack_table_);
    }
}

uint32_t Connection::AllocateStream() {
    for(int i = 1; i < streams_.size(); i++) {
        if(streams_[i].Status() == Stream::HTTP2_STREAM_IDLE) {
            streams_[i].SetStatus(Stream::HTTP2_STREAM_CLOSED);
        }
    }
    return 0;
}

void Connection::SendFrame(uint32_t streamId, Frame* frame) {
    if(streamId >= streams_.size()) {
        return;
    }

    Stream& stream = streams_[streamId];

    switch(stream.Status()) {
        case Stream::HTTP2_STREAM_IDLE : break;
        case Stream::HTTP2_STREAM_RESERVED : break;
        case Stream::HTTP2_STREAM_OPEN : break;
        case Stream::HTTP2_STREAM_HALF_CLOSED_LOCAL : break;
        case Stream::HTTP2_STREAM_HALF_CLOSED_REMOTE : break;
        case Stream::HTTP2_STREAM_CLOSED : break;
        default : break;
    }

    frame->SetStreamId(streamId);
    Frame::SendFrame(fd_, frame, hpack_table_);
}

Frame* Connection::RecvFrame() {
    Frame* frame = Frame::RecvFrame(fd_, hpack_table_);
    return frame;
}

uint32_t Connection::LastClientStreamId() {
    return streams_.size();
}

uint32_t Connection::LastServerStreamId() {
    return streams_.size();
}

Stream::HTTP2_STREAM_STATUS Connection::StreamStatus(int streamId) {
    if(streamId <= 1)
        return Stream::HTTP2_STREAM_RESERVED;
    if(streamId >= streams_.size())
        return Stream::HTTP2_STREAM_IDLE;
    return streams_[streamId].Status();
}

lhttp2::Settings& Connection::Settings() {
    return settings_;
}

void Connection::SetSettings(lhttp2::Settings settings) {
    settings_ = settings;
    hpack_table_.UpdateSize(settings_.HeaderTableSize());
}

void Connection::UseHuffman(bool use) {
    use_huffman_ = use;
}

void Connection::SendPreface() {
    ::send(fd_, preface, PREFACE_LEN, 0);
}

bool Connection::RecvPreface() {
    char buffer[PREFACE_LEN];
    int read_len;

    read_len = ::read(fd_, buffer, PREFACE_LEN);
    if(read_len != PREFACE_LEN) return false;

    for(int i = 0; i < PREFACE_LEN; i++)
        if(preface[i] != buffer[i])
            return false;

    return true;
}