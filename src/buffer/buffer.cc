#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

#include "buffer.h"

Buffer::Buffer() {
    len = 0;
    max_len = 8;
    buffer = (char *)malloc(sizeof(char) * max_len);
    memset(buffer, 0, max_len);
}

Buffer::Buffer(const unsigned int buff_len) {
    UpdateBufferSize(buff_len);
    memset(buffer, 0, max_len);
    len = 0;
}

Buffer::Buffer(const char* str) {
    UpdateBufferSize(strlen(str));
    memset(buffer, 0, max_len);
    strncpy(buffer, str, strlen(str));
    len = strlen(str);
}

Buffer::Buffer(const char* str, const unsigned int str_len) {
    UpdateBufferSize(str_len);
    memset(buffer, 0, max_len);
    memcpy(buffer, str, str_len);
    len = str_len;
}

Buffer::~Buffer() {
    if(buffer != nullptr) free(buffer);
}

void Buffer::Append(const struct Buffer& a) {
    Copy(a, len);
}

void Buffer::Append(const char* buff, const unsigned int buff_len) {
    Copy(buff, buff_len, len);
}

void Buffer::Append(const char ch) {
    Copy(ch, len);
}

void Buffer::Copy(const struct Buffer& a, const unsigned int at) {
    if(max_len < at + a.len) UpdateBufferSize(at + a.len);
    memcpy(buffer + at, a.buffer, a.len);
    len = at + a.len;
}

void Buffer::Copy(const char* buff, const unsigned int buff_len, const unsigned int at) {
    if(max_len < at + buff_len) UpdateBufferSize(at + buff_len);
    memcpy(buffer + at, buff, buff_len);
    len = at + buff_len;
}

void Buffer::Copy(const char ch, const unsigned int at) {
    Set(ch, at);
}

void Buffer::Resize(const unsigned int buff_len) {
    UpdateBufferSize(buff_len);
    len = buff_len;
}

void Buffer::Clear() {
    Resize(0);
}

unsigned int Buffer::Length() const {
    return len;
}

const char* Buffer::Address(const unsigned int idx) const {
    if(idx >= len) return nullptr;
    return buffer + idx;
}

char Buffer::Get(const unsigned int idx) const {
    if(idx >= len) return '\0';
    return buffer[idx];
}

void Buffer::Set(const char ch, const unsigned int idx) {
    SetValue(ch, 1, idx);
}

uint64_t Buffer::GetValue(const unsigned int bytes, const unsigned int idx) const {
    if(bytes > 8) return 0;
    uint64_t value = 0;
    for(int i = 0; i < bytes; i++) {
        value = (value << 8) | buffer[i + idx];
    }
    return value;
}

bool Buffer::SetValue(const uint64_t value, const int bytes, const unsigned int idx) {
    if(bytes > 8) return false;
    if(max_len < idx + bytes) UpdateBufferSize(idx + bytes);
    int offset = 8 * bytes;
    for(int i = 0; i < bytes; i++) {
        offset = offset - 8;
        buffer[i + idx] = (uint8_t)(value >> offset);
    }
    if(len < idx + bytes) len = idx + bytes;
    return true;
}

void Buffer::Print() const {
    PrintBuffer(buffer, len);
}

char& Buffer::operator[](const unsigned int i) {
    return buffer[i];
}

struct Buffer& Buffer::operator=(const struct Buffer& a) {
    UpdateBufferSize(a.len);
    memcpy(buffer, a.buffer, a.len);
    len = a.len;
    return *this;
}

struct Buffer& Buffer::operator=(const char* str) {
    UpdateBufferSize(strlen(str));
    memcpy(buffer, str, strlen(str));
    len = strlen(str);
    return *this;
}

struct Buffer& Buffer::operator+(const struct Buffer& a) {
    if(max_len < len + a.len) UpdateBufferSize(len + a.len);
    memcpy(buffer + len, a.buffer, a.len);
    len = len + a.len;
    return *this;
}

struct Buffer& Buffer::operator+(const char* str) {
    if(max_len < len + strlen(str)) UpdateBufferSize(len + strlen(str));
    memcpy(buffer + len, str, strlen(str));
    len = len + strlen(str);
    return *this;
}

struct Buffer& Buffer::operator+(const char ch) {
    if(max_len < len + 1) UpdateBufferSize(len + 1);
    buffer[len] = ch;
    len = len + 1;
    return *this;
}

struct Buffer& Buffer::operator+=(const struct Buffer& a) {
    if(max_len < len + a.len) UpdateBufferSize(len + a.len);
    memcpy(buffer + len, a.buffer, a.len);
    len = len + a.len;
    return *this;
}

struct Buffer& Buffer::operator+=(const char* str) {
    if(max_len < len + strlen(str)) UpdateBufferSize(len + strlen(str));
    memcpy(buffer + len, str, strlen(str));
    len = len + strlen(str);
    return *this;
}

struct Buffer& Buffer::operator+=(const char ch) {
    if(max_len < len + 1) UpdateBufferSize(len + 1);
    buffer[len] = ch;
    len = len + 1;
    return *this;
}

void Buffer::UpdateBufferSize(const unsigned int min) {
    max_len = 8;
    while(max_len < min) {
        max_len = max_len * 2;
    }
    buffer = (char *)realloc(buffer, sizeof(char) * max_len);
}

void Buffer::PrintBuffer(const char* buff, const int len) {
    int i, j, cnt;
    for(i = 0, cnt = 0; i < len; i++) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int)((unsigned char)buff[i]) << ' ';
        cnt++;
        if(cnt % 8 == 0 || i == len - 1) {
            for(j = 0; j < 8 - cnt; j++) std::cout << "   ";
            for(; cnt > 0; cnt--) {
                if(buff[i - cnt + 1] >= 0x20 && buff[i - cnt + 1] <= 0x7E) std::cout << buff[i - cnt + 1];
                else std::cout << '.';
            }
            std::cout << std::endl;
            cnt = 0;
        }
    }
}