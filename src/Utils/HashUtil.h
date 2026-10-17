//
// Created by engin on 25/10/2023.
//

#ifndef LIMONENGINE_HASHUTIL_H
#define LIMONENGINE_HASHUTIL_H

#include <string>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <glm/glm.hpp>
#include "limonAPI/util/HashUtil.h"

class HashUtil {
public:
    HashUtil() = delete;

    class HashedString {
    public :
        const uint64_t hash;
        const std::string text;
        explicit HashedString(const std::string& text): hash(::hash(text)), text(text){
            auto textIterator = allHashedStrings.find(hash);
            if(textIterator != allHashedStrings.end() && textIterator->second != text) {
                std::cerr << "Hash collision found , both [" << text << "] and [" << textIterator->second << "] generated same hash " << hash << std::endl;
            } else {
                allHashedStrings[hash] = text;
            }
        }
    };

    static uint64_t hashString(const std::string& text) {
        uint64_t hash = ::hash(text);
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

    //fixed bytes for a hash that is saved, so it is the same on every compiler: integers little endian, floats by their bits
    static void appendUint32(std::string &bytes, uint32_t value) {
        for (uint32_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
            bytes.push_back(static_cast<char>((value >> (8 * byteIndex)) & 0xFF));
        }
    }

    static void appendFloat(std::string &bytes, float value) {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        appendUint32(bytes, bits);
    }

    static void appendVec3(std::string &bytes, const glm::vec3 &value) {
        appendFloat(bytes, value.x);
        appendFloat(bytes, value.y);
        appendFloat(bytes, value.z);
    }

private:
    static std::unordered_map<uint64_t, std::string> allHashedStrings;

};

#endif //LIMONENGINE_HASHUTIL_H
