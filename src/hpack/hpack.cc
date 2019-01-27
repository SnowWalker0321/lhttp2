#include <stdint.h>
#include <cstdlib>
#include <string>
#include <iostream>

#include "hpack.h"
#include "huffman.h"

using namespace hpack;

static const struct HeaderField static_table[STATIC_TABLE_SIZE] = {
    {"", ""},                               // 0
    {":authority", ""},                     // 1
    {":method", "GET"},                     // 2
    {":method", "POST"},                    // 3
    {":path", "/"},                         // 4
    {":path", "/index.html"},               // 5
    {":scheme", "http"},                    // 6
    {":scheme", "https"},                   // 7
    {":status", "200"},                     // 8
    {":status", "204"},                     // 9
    {":status", "206"},                     // 10
    {":status", "304"},                     // 11
    {":status", "400"},                     // 12
    {":status", "404"},                     // 13
    {":status", "500"},                     // 14
    {"accept-charset", ""},                 // 15
    {"accept-encoding", "gzip, deflate"},   // 16
    {"accept-language", ""},                // 17
    {"accept-ranges", ""},                  // 18
    {"accept", ""},                         // 19
    {"access-control-allow-origin", ""},    // 20
    {"age", ""},                            // 21
    {"allow", ""},                          // 22
    {"authorization", ""},                  // 23
    {"cache-control", ""},                  // 24
    {"content-disposition", ""},            // 25
    {"content-encoding", ""},               // 26
    {"content-language", ""},               // 27
    {"content-length", ""},                 // 28
    {"content-location", ""},               // 29
    {"content-range", ""},                  // 30
    {"content-type", ""},                   // 31
    {"cookie", ""},                         // 32
    {"date", ""},                           // 33
    {"etag", ""},                           // 34
    {"expect", ""},                         // 35
    {"expires", ""},                        // 36
    {"from", ""},                           // 37
    {"host", ""},                           // 38
    {"if-match", ""},                       // 39
    {"if-modified-since", ""},              // 40
    {"if-none-match", ""},                  // 41
    {"if-range", ""},                       // 42
    {"if-unmodified-since", ""},            // 43
    {"last-modified", ""},                  // 44
    {"link", ""},                           // 45
    {"location", ""},                       // 46
    {"max-forwards", ""},                   // 47
    {"proxy-authenticate", ""},             // 48
    {"proxy-authorization", ""},            // 49
    {"range", ""},                          // 50
    {"referer", ""},                        // 51
    {"refresh", ""},                        // 52
    {"retry-after", ""},                    // 53
    {"server", ""},                         // 54
    {"set-cookie", ""},                     // 55
    {"strict-transport-security", ""},      // 56
    {"transfer-encoding", ""},              // 57
    {"user-agend", ""},                     // 58
    {"vary", ""},                           // 59
    {"via", ""},                            // 60
    {"www-authenticate", ""},               // 61
};

static const uint8_t prefix_max[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

static void EncodeInteger(Buffer& buff, uint32_t i, uint8_t prefix_length, uint8_t prefix_dummy) {
    if(prefix_length <= 0 || prefix_length > 8) {
        buff.Clear();
        return;
    }

    if(i < prefix_max[prefix_length]) {
        buff.Resize(1);
        buff.Set((prefix_dummy & ~prefix_max[prefix_length]) | i, 0);
    }
    else {
        int idx = 1;
        buff.Resize(2 + (i - prefix_max[prefix_length]) / 128);
        buff.Set((prefix_dummy & ~prefix_max[prefix_length]) | prefix_max[prefix_length], 0);
        i = i - prefix_max[prefix_length];
        while(i >= 128) {
            buff.Set(i % 128 + 128, idx++);
            i = i / 128;
        }
        buff.Set(i, idx);
    }
}

static uint32_t DecodeInteger(const Buffer& buff, uint32_t& offset, uint8_t prefix_length) {
    if(prefix_length <= 0 || prefix_length > 8) {
        return 0;
    }

    uint32_t i = buff.Get(offset++) & prefix_max[prefix_length];

    if(i >= prefix_max[prefix_length]) {
        uint32_t p = 1;
        do {
            if(offset >= buff.Length()) {
                return 0;
            }
            i = i + (buff.Get(offset & 127)) * p;
            p = p * 128;
        } while((buff.Get(offset++) & 128) == 128);
    }

    return i;
}

static void AppendToTable(std::vector<HeaderField>& dynamic_table, uint32_t dynamic_table_size_max, HeaderField header) {
    std::vector<HeaderField>::iterator it;
    it = dynamic_table.begin();
    dynamic_table.insert(it, header);
    
    while(dynamic_table.size() > dynamic_table_size_max) {
        dynamic_table.pop_back();
    }
}

static uint32_t FindFromTable(std::vector<HeaderField>& dynamic_table, std::string name, std::string value) {
    uint32_t i, total_size = STATIC_TABLE_SIZE + dynamic_table.size();
    bool compare_value = false;
    HeaderField header;

    if(value.compare("") != 0)
        compare_value = true;

    for(i = 1; i < total_size; i++) {
        if(i < STATIC_TABLE_SIZE) header = static_table[i];
        else header = dynamic_table[i - STATIC_TABLE_SIZE];
        if(header.Name().compare(name) == 0) {
            if(compare_value == false || (compare_value == true && header.Value().compare(value) == 0)) {
                return i;
            }
        }
    }

    return 0;
}

bool Table::Encode(Buffer& encoded_buffer, std::vector<HeaderFieldRepresentation> header_list, bool update) {
    uint32_t idx;
    Buffer encode_int, huff;
    std::vector<HeaderFieldRepresentation>::iterator it = header_list.begin();
    std::vector<HeaderField>& update_table = dynamic_table_;

    if(update == false) {
        std::vector<HeaderField> temp_table = dynamic_table_;
        update_table = temp_table;
    }

    encoded_buffer.Clear();

    for(; it != header_list.end(); it++) {
        if(it->Type() == HeaderField::INDEXED_HEADER_FIELD) {
            idx = Find(it->Field().Name(), it->Field().Value());
            if(idx == 0) {
                it->Type() = HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
                it--;
                continue;
            }
            EncodeInteger(encode_int, idx, 7, 0x80);
            encoded_buffer.Append(encode_int);
        }
        else {
            idx = Find(it->Field().Name(), "");
            if(idx == 0) {
                if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING)
                    encoded_buffer.Append(0x40);
                else if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_WITHOUT_INDEXING)
                    encoded_buffer.Append(0x00);
                else if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_NEVER_INDEXED)
                    encoded_buffer.Append(0x10);
                
                if(it->Field().NameUseHuffman() == true) {
                    Huffman::GetInstance().Encode(huff, it->Field().Name().length());
                    EncodeInteger(encode_int, huff.Length(), 7, 0x80);
                    encoded_buffer.Append(encode_int);
                    encoded_buffer.Append(huff);
                }
                else {
                    EncodeInteger(encode_int, it->Field().Name().length(), 7, 0);
                    encoded_buffer.Append(encode_int);
                    encoded_buffer.Append(it->Field().Name().c_str());
                }
            } else {
                if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING)
                    EncodeInteger(encode_int, idx, 6, 0x40);
                else if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_WITHOUT_INDEXING)
                    EncodeInteger(encode_int, idx, 4, 0x00);
                else if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_NEVER_INDEXED)
                    EncodeInteger(encode_int, idx, 4, 0x10);
                encoded_buffer.Append(encode_int);
            }

            if(it->Field().ValueUseHuffman() == true) {
                Huffman::GetInstance().Encode(huff, it->Field().Value().length());
                EncodeInteger(encode_int, huff.Length(), 7, 0x80);
                encoded_buffer.Append(encode_int);
                encoded_buffer.Append(huff);
            }
            else {
                EncodeInteger(encode_int, it->Field().Value().length(), 7, 0);
                encoded_buffer.Append(encode_int);
                encoded_buffer.Append(it->Field().Value().c_str());
            }
        }
    }

    return true;
}

bool Table::Decode(std::vector<HeaderFieldRepresentation>& header_list, const Buffer& buff, bool update_table) {
    uint32_t offset = 0, idx, name_len, value_len;
    bool name_huff, value_huff;
    char first;
    Buffer name, value;

    HeaderFieldRepresentation header;

    while(offset < buff.Length()) {
        first = buff.Get(offset);

        // Indexed Header Field
        if((first & 0x80) == 0x80) {
            idx = DecodeInteger(buff, offset, 7);

            if(idx == 0) {
                return false;
            }

            if(idx < STATIC_TABLE_SIZE) {
                header.Field().SetName(static_table[idx].Name());
                header.Field().SetValue(static_table[idx].Value());
            }
            else {
                if(idx - STATIC_TABLE_SIZE >= dynamic_table_.size()) return false;
                header.Field().SetName(dynamic_table_[idx - STATIC_TABLE_SIZE].Name());
                header.Field().SetValue(dynamic_table_[idx - STATIC_TABLE_SIZE].Value());
            }
            header.Type() = HeaderField::INDEXED_HEADER_FIELD;
        }
        
        // Literal Header Field
        else {
            // Literal Header Field with Incremental Indexing
            if((first & 0x40) == 0x40) {
                idx = DecodeInteger(buff, offset, 6);
                header.Type() = HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
            }

            // Maximum Dynamic Table Size Change
            else if((first & 0x20) == 0x20) {
                uint32_t size = DecodeInteger(buff, offset, 5);
                UpdateSize(size);
                continue;
            }

            // Literal Header Field Never Indexed
            else if((first & 0x10) == 0x10) {
                idx = DecodeInteger(buff, offset, 4);
                header.Type() = HeaderField::LITERAL_HEADER_FIELD_NEVER_INDEXED;
            }

            // Literal Header Field without Indexing
            else {
                idx = DecodeInteger(buff, offset, 4);
                header.Type() = HeaderField::LITERAL_HEADER_FIELD_WITHOUT_INDEXING;
            }

            if(idx > 0) {
                if(idx < STATIC_TABLE_SIZE)
                    header.Field().SetName(static_table[idx].Name());
                else {
                    if(idx - STATIC_TABLE_SIZE >= dynamic_table_.size()) return false;
                    header.Field().SetName(dynamic_table_[idx - STATIC_TABLE_SIZE].Name);
                }
            }
            else {
                header.Field().SetNameUseHuffman((buff.Get(offset) & 128) == 128);
                name_len = DecodeInteger(buff, offset, 7);
                if(header.Field().NameUseHuffman() == true) {
                    if(Huffman::GetInstance().Decode(name, Buffer(buff.Address(offset), name_len)) == false) return false;
                    header.Field().SetName(std::string(name.Address(), name.Length()));
                }
                else {
                    header.Field().SetName(std::string(buff.Address(offset), name_len));
                }
                offset = offset + name_len;
            }

            header.Field().SetValueUseHuffman((buff.Get(offset) & 128) == 128);
            value_len = DecodeInteger(buff, offset, 7);
            if(header.Field().ValueUseHuffman() == true) {
                if(Huffman::GetInstance().Decode(value, Buffer(buff.Address(offset), value_len)) == false) return false;
                header.Field().SetValue(std::string(value.Address(), value.Length()));
            }
            else {
                header.Field().SetValue(std::string(buff.Address(offset), value_len));
            }
            offset = offset + value_len;
        }

        header_list.push_back(header);
        if(header.Type() == HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
            AppendToTable(dynamic_table_, dynamic_table_size_max_, header.Field());
        }
    }

    return true;
}

void Table::Update(std::vector<HeaderFieldRepresentation> header_list) {
    std::vector<HeaderFieldRepresentation>::iterator it = header_list.begin();
    for(; it != header_list.end(); it++) {
        if(it->Type() == HeaderField::LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
            Append(it->Field());
        }
    }
}

void Table::UpdateSize(uint32_t size) {
    dynamic_table_size_max_ = size;
    while(dynamic_table_.size() > dynamic_table_size_max_) {
        dynamic_table_.pop_back();
    }
}

void Table::Print() {
    for(uint32_t i = 0; i < dynamic_table_.size(); i++) {
        std::cout << "(" << STATIC_TABLE_SIZE + i << ") " << dynamic_table_[i].Name() << ":" << dynamic_table_[i].Value() << std::endl;
    }
}

void Table::Append(HeaderField header) {
    AppendToTable(dynamic_table_, dynamic_table_size_max_, header);
}

uint32_t Table::Find(const std::string name, const std::string value) {
    return FindFromTable(dynamic_table_, name, value);
}