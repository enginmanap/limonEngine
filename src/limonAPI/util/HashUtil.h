//
// Created by engin on 26/02/2025.
//

#ifndef HASHUTIL_H
#define HASHUTIL_H

#include "consthash/include/consthash/cityhash64.hxx"
#include <string>

struct constexpr_str {
    char const* str;
    std::size_t size;

    // can only construct from a char[] literal
    template <std::size_t N>
    constexpr constexpr_str(char const (&s)[N])
            : str(s)
            , size(N - 1) // not count the trailing nul
    {}
};

constexpr uint64_t HASH(constexpr_str text) {
    return consthash::city64(text.str, text.size);
}

static_assert(HASH("test") == consthash::city64("test", 4), "HASH function did not run at compile time");


inline uint64_t hash(std::string text) {
    return consthash::city64(text.c_str(), text.length());
}

#endif //HASHUTIL_H
