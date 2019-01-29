#ifndef _LHTTP2_CONNECTION_H
#define _LHTTP2_CONNECTION_H

#include <vector>
#include <stdint.h>

#include "stream.h"
#include "frame.h"
#include "settings.h"
#include "hpack/hpack.h"

namespace lhttp2 {
    class Connection {
    public:
        typedef enum _ENDPOINT_TYPE {
            ENDPOINT_CLIENT,
            ENDPOINT_SERVER,
        } ENDPOINT_TYPE;

        Connection(int fd, ENDPOINT_TYPE type, lhttp2::Settings settings = lhttp2::Settings());

        uint32_t AllocateStream();

        void SendFrame(uint32_t streamId, Frame* frame);
        Frame* RecvFrame();

        uint32_t LastClientStreamId();
        uint32_t LastServerStreamId();
        Stream::HTTP2_STREAM_STATUS StreamStatus(int streamId);

        lhttp2::Settings& Settings();
        void SetSettings(lhttp2::Settings settings);

        void UseHuffman(bool use);

    private:
        void SendPreface();
        bool RecvPreface();

        int fd_;
        ENDPOINT_TYPE type_;
        std::vector<Stream> streams_;
        uint32_t window_size_ = 65535;
        lhttp2::Settings settings_;
        hpack::Table hpack_table_;
        bool use_huffman_ = true;
    };

    class Server : public Connection {
        void Listen();
    };

    class Client : public Connection {
        void Connect();
    };
};

#endif