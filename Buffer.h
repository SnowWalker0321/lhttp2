#ifndef _BUFFER_H_
#define _BUFFER_H_

#define GET_1BIT(buff, mask)    (((*(uint8_t *)(buff)) >> 7) & mask)
#define GET_1B_INT(buff, mask)  ((((*(uint8_t *)(buff)))) & mask)
#define GET_2B_INT(buff, mask)  ((((uint16_t)(*(uint8_t *)(buff)) << 8) | \
                                ((uint16_t)(*(uint8_t *)(buff + 1)))) & mask)
#define GET_3B_INT(buff, mask)  ((((uint32_t)(*(uint8_t *)(buff)) << 16) | \
                                ((uint32_t)(*(uint8_t *)(buff + 1)) << 8) | \
                                ((uint32_t)(*(uint8_t *)(buff + 2)))) & mask)
#define GET_4B_INT(buff, mask)  ((((uint32_t)(*(uint8_t *)(buff)) << 24) | \
                                ((uint32_t)(*(uint8_t *)(buff + 1)) << 16) | \
                                ((uint32_t)(*(uint8_t *)(buff + 2)) << 8) | \
                                ((uint32_t)(*(uint8_t *)(buff + 3)))) & mask)
#define GET_8B_INT(buff, mask)  ((((uint64_t)(*(uint8_t *)(buff)) << 56) | \
                                ((uint64_t)(*(uint8_t *)(buff + 1)) << 48) | \
                                ((uint64_t)(*(uint8_t *)(buff + 2)) << 40) | \
                                ((uint64_t)(*(uint8_t *)(buff + 3)) << 32) | \
                                ((uint64_t)(*(uint8_t *)(buff + 4)) << 24) | \
                                ((uint64_t)(*(uint8_t *)(buff + 5)) << 16) | \
                                ((uint64_t)(*(uint8_t *)(buff + 6)) << 8) | \
                                ((uint64_t)(*(uint8_t *)(buff + 7)))) & mask)

#define SET_1B_INT(buff, val, mask) ((char *)buff)[0] = (((uint8_t)(val)) & ((uint8_t)(mask)));
#define SET_2B_INT(buff, val, mask) ((char *)buff)[0] = (((uint8_t)(val >> 8)) & ((uint8_t)(mask >> 8))); \
                                    ((char *)buff)[1] = (((uint8_t)(val)) & ((uint8_t)(mask)));
#define SET_3B_INT(buff, val, mask) ((char *)buff)[0] = (((uint8_t)(val >> 16)) & ((uint8_t)(mask >> 16))); \
                                    ((char *)buff)[1] = (((uint8_t)(val >> 8)) & ((uint8_t)(mask >> 8))); \
                                    ((char *)buff)[2] = (((uint8_t)(val)) & ((uint8_t)(mask)));
#define SET_4B_INT(buff, val, mask) ((char *)buff)[0] = (((uint8_t)(val >> 24)) & ((uint8_t)(mask >> 24))); \
                                    ((char *)buff)[1] = (((uint8_t)(val >> 16)) & ((uint8_t)(mask >> 16))); \
                                    ((char *)buff)[2] = (((uint8_t)(val >> 8)) & ((uint8_t)(mask >> 8))); \
                                    ((char *)buff)[3] = (((uint8_t)(val)) & ((uint8_t)(mask)));
#define SET_8B_INT(buff, val, mask) ((char *)buff)[0] = (((uint8_t)(val >> 56)) & ((uint8_t)(mask >> 56))); \
                                    ((char *)buff)[1] = (((uint8_t)(val >> 48)) & ((uint8_t)(mask >> 48))); \
                                    ((char *)buff)[2] = (((uint8_t)(val >> 40)) & ((uint8_t)(mask >> 40))); \
                                    ((char *)buff)[3] = (((uint8_t)(val >> 32)) & ((uint8_t)(mask >> 32))); \
                                    ((char *)buff)[4] = (((uint8_t)(val >> 24)) & ((uint8_t)(mask >> 24))); \
                                    ((char *)buff)[5] = (((uint8_t)(val >> 16)) & ((uint8_t)(mask >> 16))); \
                                    ((char *)buff)[6] = (((uint8_t)(val >> 8)) & ((uint8_t)(mask >> 8))); \
                                    ((char *)buff)[7] = (((uint8_t)(val)) & ((uint8_t)(mask)));

struct Buffer {
    char* buffer;
    unsigned int len;

    Buffer();
    Buffer(const unsigned int buff_len);
    Buffer(const char *str);
    ~Buffer();

    void copy_buffer(const struct Buffer& a, const unsigned int at = 0);
    void copy_buffer(const char* buff, const unsigned int buff_len, const unsigned int at = 0);
    void resize_buffer(const unsigned int buff_len);
    void print_buffer();

    char& operator[](const unsigned int i);
    struct Buffer& operator=(const struct Buffer& a);
    struct Buffer& operator=(const char* str);
    struct Buffer& operator+(const struct Buffer& a);
    struct Buffer& operator+(const char* str);
    struct Buffer& operator+=(const struct Buffer& a);
    struct Buffer& operator+=(const char* str);
};

#endif