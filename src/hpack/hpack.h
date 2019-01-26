#ifndef _HPACK_H_
#define _HPACK_H_

#define STATIC_TABLE_SIZE 62
#define DYNAMIC_TABLE_SIZE_MAX 4096

#include <vector>
#include <string>

#include "../buffer/Buffer.h"

namespace hpack {
    struct HeaderField {
        public:
            typedef enum _HEADER_FIELD_TYPE {
                INDEXED_HEADER_FIELD = 0,
                LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING,
                LITERAL_HEADER_FIELD_WITHOUT_INDEXING,
                LITERAL_HEADER_FIELD_NEVER_INDEXED,
            } HEADER_FIELD_TYPE;

            HeaderField();
            HeaderField(std::string name, std::string value);
            HeaderField(bool name_use_huffman, bool value_use_huffman, std::string name, std::string value);

            bool NameUseHuffman() const;
            bool ValueUseHuffman() const;

            bool SetNameUseHuffman(bool use);
            bool SetValueUseHuffman(bool use);

            const std::string& Name() const;
            const std::string& Value() const;

            void SetName(const std::string name);
            void SetValue(const std::string value);

        private:
            bool name_use_huffman_;
            bool value_use_huffman_;
            std::string name_;
            std::string value_;
    };

    struct HeaderFieldRepresentation {
        public:
            HeaderField& Field();
            HeaderField::HEADER_FIELD_TYPE& Type();

        private:
            HeaderField header_field_;
            HeaderField::HEADER_FIELD_TYPE type_;
    };

    class Table {
    public:
        bool Encode(Buffer& encoded_buffer, std::vector<HeaderFieldRepresentation> header_list, bool update_table = true);
        bool Decode(std::vector<HeaderFieldRepresentation>& header_list, const Buffer& buff, bool update_table = true);

        void Update(std::vector<HeaderFieldRepresentation> header_list);
        void UpdateSize(uint32_t size);

        void Print();

    private:
        void Append(HeaderField header);
        uint32_t Find(const std::string name, const std::string value = "");

        uint32_t dynamic_table_size_max_;
        std::vector<HeaderField> dynamic_table_;
    };
}

#endif