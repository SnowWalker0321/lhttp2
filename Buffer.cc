#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

#include "Buffer.h"

Buffer::Buffer() {
        buffer = nullptr;
        len = 0;
}

Buffer::Buffer(const unsigned int buff_len) {
    buffer = (char *)malloc(sizeof(char) * buff_len);
    memset(buffer, 0, buff_len);
    len = buff_len;
}

Buffer::Buffer(const char *str) {
    buffer = (char *)malloc(strlen(str) + 1);
    memset(buffer, 0, strlen(str));
    strcpy(buffer, str);
    len = strlen(str);
}

Buffer::~Buffer() {
    if(buffer != nullptr) free(buffer);
}

void Buffer::copy_buffer(const struct Buffer& a, const unsigned int at) {
    if(len < at + a.len) {
        buffer = (char *)realloc(buffer, at + a.len);
        len = at + a.len;
    }
    memcpy(buffer + at, a.buffer, a.len);
}

void Buffer::copy_buffer(const char* buff, const unsigned int buff_len, const unsigned int at) {
    if(len < at + buff_len) {
        buffer = (char *)realloc(buffer, at + buff_len);
        len = at + buff_len;
    }
    memcpy(buffer + at, buff, buff_len);
}

void Buffer::resize_buffer(const unsigned int buff_len) {
    buffer = (char *)realloc(buffer, buff_len);
    len = buff_len;
}

void Buffer::print_buffer() {
    int i, j, cnt;
    for(i = 0, cnt = 0; i < len; i++) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int)((unsigned char)buffer[i]) << ' ';
        cnt++;
        if(cnt % 8 == 0 || i == len - 1) {
            for(j = 0; j < 8 - cnt; j++) std::cout << "   ";
            for(; cnt > 0; cnt--) {
                if(buffer[i - cnt + 1] >= 0x20 && buffer[i - cnt + 1] <= 0x7E) std::cout << buffer[i - cnt + 1];
                else std::cout << '.';
            }
            std::cout << std::endl;
            cnt = 0;
        }
    }
}

char& Buffer::operator[](const unsigned int i) {
    return buffer[i];
}

struct Buffer& Buffer::operator=(const struct Buffer& a) {
    buffer = (char *)realloc(buffer, a.len);
    memcpy(buffer, a.buffer, a.len);
    len = a.len;
    return *this;
}

struct Buffer& Buffer::operator=(const char* str) {
    buffer = (char *)realloc(buffer, strlen(str));
    memcpy(buffer, str, strlen(str));
    len = strlen(str);
    return *this;
}

struct Buffer& Buffer::operator+(const struct Buffer& a) {
    buffer = (char *)realloc(buffer, len + a.len);
    memcpy(buffer + len, a.buffer, a.len);
    len = len + a.len;
    return *this;
}

struct Buffer& Buffer::operator+(const char* str) {
    buffer = (char *)realloc(buffer, len + strlen(str));
    memcpy(buffer + len, str, strlen(str));
    len = len + strlen(str);
    return *this;
}

struct Buffer& Buffer::operator+=(const struct Buffer& a) {
    buffer = (char *)realloc(buffer, len + a.len);
    memcpy(buffer + len, a.buffer, a.len);
    len = len + a.len;
    return *this;
}

struct Buffer& Buffer::operator+=(const char* str) {
    buffer = (char *)realloc(buffer, len + strlen(str));
    memcpy(buffer + len, str, strlen(str));
    len = len + strlen(str);
    return *this;
}