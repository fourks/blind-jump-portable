#pragma once

#include "bitvector.hpp"
#include "function.hpp"
#include "number/numeric.hpp"
#include "string.hpp"
#include <optional>


namespace utf8 {

using Codepoint = u32;


inline bool
    scan(Function<32, void(const Codepoint& cp, const char* raw, int index)>
             callback,
         const char* data,
         size_t len)
{
    size_t index = 0;
    while (index < len) {

        char raw[5] = {'\0', '\0', '\0', '\0', '\0'};
        const Bitvector<8> parsed(data[index]);
        if (parsed[7] == 0) {
            raw[0] = data[index];
            callback(data[index], raw, index);
            index += 1;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 0) {
            raw[0] = data[index];
            raw[1] = data[index + 1];
            callback(data[index] | (data[index + 1] << 8), raw, index);
            index += 2;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                   parsed[4] == 0) {
            raw[0] = data[index];
            raw[1] = data[index + 1];
            raw[2] = data[index + 2];
            callback(data[index] | (data[index + 1] << 8) |
                         (data[index + 2] << 16),
                     raw,
                     index);
            index += 3;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                   parsed[4] == 1 and parsed[3] == 0) {
            raw[0] = data[index];
            raw[1] = data[index + 1];
            raw[2] = data[index + 2];
            raw[3] = data[index + 3];
            callback(data[index] | (data[index + 1] << 8) |
                         (data[index + 2] << 16) | (data[index + 3] << 24),
                     raw,
                     index);
            index += 4;
        } else {
            return false;
        }
    }
    return true;
}


// This returns the first utf8 Codepoint in a string, meant mostly as a utility
// for creating codepoint literals from strings. Unfortunately, C++ doesn't
// offer unicode character literals... I guess I could specify them in hex, but
// that's no fun (and not so easy for other people to read).
inline Codepoint getc(const char* data)
{
    std::optional<Codepoint> front;
    scan(
        [&front](const Codepoint& cp, const char*, int) {
            if (not front)
                front = cp;
        },
        data,
        str_len(data));

    if (front) {
        return *front;
    } else {
        return '\0';
    }
}


inline size_t len(const char* data)
{
    size_t ret = 0;
    scan([&ret](const Codepoint&, const char*, int) { ++ret; },
         data,
         str_len(data));
    return ret;
}


//
// BufferedStr is a complicated piece of code meant to speed up random access
// into a utf-8 string. Lots of tradeoffs in this implementation... but
// basically, on the Gameboy Advance, we cannot fit a parsed representation of a
// large utf-8 string in memory, so we have this adapter class, which loads
// small chunks of codepoints from the encoded string.
//
class BufferedStr {
    // public:
    //     using Utf8Index = int;
    //     using RawIndex = int;

    //     enum {
    //         table_size = 8
    //     };

    //     using LookupTable = std::array<std::pair<Utf8Index, RawIndex>, table_size>;

private:
    // LookupTable lookup_table_;

    const char* const str_;

    static constexpr const int index_count_ = 300;
    utf8::Codepoint data_[index_count_];
    int index_start_;
    int str_len_;

    // inline void populate_table()
    // {
    //     const auto len = utf8::len(str_);

    //     const auto block_size = len / table_size;

    //     int i = 0;
    //     utf8::scan([block_size, &i, this](const utf8::Codepoint&, const char*, int index) {
    //                    if (i > 0 and i % block_size == 0) {
    //                        lookup_table_[i / block_size] = {i, index};
    //                    }
    //                    i++;
    //                },
    //         str_,
    //         str_len_);
    // }

    inline void load_chunk(int index)
    {
        size_t start_offset = 0;
        int i = 0;
        // for (auto& elem : reversed(lookup_table_)) {
        //     if (elem.first <= index) {
        //         start_offset = elem.second;
        //         i = elem.first;
        //     }
        // }
        size_t stop_offset = str_len_;
        // for (auto& elem : lookup_table_) {
        //     if (elem.first > index + index_count_) {
        //         stop_offset = elem.second;
        //     }
        // }

        utf8::scan(
            [this, &i, &index](const utf8::Codepoint& cp, const char*, int) {
                if (i >= index and i < index + index_count_) {
                    data_[i - index] = cp;
                }
                i++;
            },
            str_ + start_offset,
            stop_offset - start_offset);

        index_start_ = index;
    }

public:
    inline BufferedStr(const char* str, int len)
        : str_(str), index_start_(0), str_len_(len)
    {
        load_chunk(0);
    }

    inline utf8::Codepoint get(int index)
    {
        if (not(index >= index_start_ and
                index < index_start_ + index_count_)) {
            load_chunk(index);
        }

        return data_[index - index_start_];
    }
};


} // namespace utf8
