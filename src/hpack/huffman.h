#ifndef _HPACK_HUFFMAN_H_
#define _HPACK_HUFFMAN_H_

#include <stdint.h>

#include "../buffer/Buffer.h"

#define HUFFMAN_CODE_SIZE 257

namespace hpack {
    class Huffman {
    public:
        static Huffman& GetInstance();
        static bool Encode(Buffer& encoded_buffer, const Buffer& string);
        static bool Decode(Buffer& decoded_buffer, const Buffer& code);

        Huffman(Huffman const&) = delete;
        void operator=(Huffman const&) = delete;

    private:
        Huffman();
        ~Huffman();

        struct node {
            struct node *left = nullptr, *right = nullptr;
            int value = -1;
        };
        
        static struct node root_;
    };
}

#endif