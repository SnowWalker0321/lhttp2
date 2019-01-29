#ifndef _LHTTP2_BUFFER_H_
#define _LHTTP2_BUFFER_H_

#include <cstdint>

struct Buffer {
public:
    Buffer();
    Buffer(const unsigned int buff_len);
    Buffer(const char* str);
    Buffer(const char* str, const unsigned int str_len);
    ~Buffer();

    void Append(const struct Buffer& a);
    void Append(const char* buff, const unsigned int buff_len);
    void Append(const char ch);

    void Copy(const struct Buffer& a, const unsigned int at = 0);
    void Copy(const char* buff, const unsigned int buff_len, const unsigned int at = 0);
    void Copy(const char ch, const unsigned int at = 0);

    unsigned int Length() const;

    void Resize(const unsigned int buff_len);
    void Clear();

    const char* Address(const unsigned int idx = 0) const;

    char Get(const unsigned int idx) const;
    void Set(const char ch, const unsigned int idx);

    uint64_t GetValue(const unsigned int bytes, const unsigned int idx) const;
    bool SetValue(const uint64_t value, const int bytes, const unsigned int idx);

    void Print() const;

    char& operator[](const unsigned int i);

    struct Buffer& operator=(const struct Buffer& a);
    struct Buffer& operator=(const char* str);

    struct Buffer& operator+(const struct Buffer& a);
    struct Buffer& operator+(const char* str);
    struct Buffer& operator+(const char ch);

    struct Buffer& operator+=(const struct Buffer& a);
    struct Buffer& operator+=(const char* str);
    struct Buffer& operator+=(const char ch);

    static void PrintBuffer(const char* buff, const int len);

private:
    void UpdateBufferSize(const unsigned int min);

    char* buffer = nullptr;
    unsigned int len = 0;
    unsigned int max_len = 8;
};

struct uint24_t {
public:
    struct uint24_t& operator=(const uint32_t& value);

private:
    uint32_t value_ : 24;
};

struct uint40_t {
public:
    struct uint40_t& operator=(const uint64_t& value);

private:
    uint64_t value_ : 40;
};

struct uint48_t {
public:
    struct uint48_t& operator=(const uint64_t& value);

private:
    uint64_t value_ : 48;
};

struct uint52_t {
public:
    struct uint52_t& operator=(const uint64_t& value);

private:
    uint64_t value_ : 52;
};

#endif