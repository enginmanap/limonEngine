//
// Created by engin on 25/10/2023.
//

#ifndef LIMONENGINE_HASHUTIL_H
#define LIMONENGINE_HASHUTIL_H

#include <string>
#include <unordered_map>
#include <iostream>
#include "consthash/include/consthash/cityhash64.hxx"

class HashUtil {
public:
    class HashedString {
    public :
        const uint64_t hash;
        const std::string text;
        explicit HashedString(const std::string& text): hash(consthash::city64(text.c_str(), text.length())), text(text){
            auto textIterator = allHashedStrings.find(hash);
            if(textIterator != allHashedStrings.end() && textIterator->second != text) {
                std::cerr << "Hash collision found , both [" << text << "] and [" << textIterator->second << "] generated same hash " << hash << std::endl;
            } else {
                allHashedStrings[hash] = text;
            }
        }
    };

    static uint64_t hashString(const std::string& text) {
        uint64_t hash = consthash::city64(text.c_str(), text.length());
        auto textIterator = allHashedStrings.find(hash);
        if(textIterator != allHashedStrings.end() && textIterator->second != text) {
            std::cerr << "Hash collision found , both [" << text << "] and [" << textIterator->second << "] generated same hash " << hash << std::endl;
        } else {
            allHashedStrings[hash] = text;
        }
        return hash;
    }

    static std::unordered_map<uint64_t, std::string >getAllHashedStrings() {
        return allHashedStrings;
    }

private:
    static std::unordered_map<uint64_t, std::string> allHashedStrings;

};

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

inline uint64_t hash(std::string text) {
    return consthash::city64(text.c_str(), text.length());
}

static_assert(HASH("test") == consthash::city64("test", 4), "HASH function did not run at compile time");
#endif //LIMONENGINE_HASHUTIL_H
