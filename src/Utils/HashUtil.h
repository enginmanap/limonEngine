//
// Created by engin on 25/10/2023.
//

#ifndef LIMONENGINE_HASHUTIL_H
#define LIMONENGINE_HASHUTIL_H

#include <string>
#include <unordered_map>
#include <iostream>
#include "../../libs/cityhash/src/city.h"

class HashUtil {
public:
    struct HashedString {
        const uint64_t hash;
        const std::string text;
        HashedString(const std::string& text): hash(CityHash64(text.c_str(), text.length())), text(text){
            auto textIterator = allHashedStrings.find(hash);
            if(textIterator != allHashedStrings.end() && textIterator->second != text) {
                std::cerr << "Hash collision found , both [" << text << "] and [" << textIterator->second << "] generated same hash " << hash << std::endl;
            } else {
                allHashedStrings[hash] = text;
            }

        }
    };

    static uint64_t hashString(const std::string& text) {
        uint64_t hash = CityHash64(text.c_str(), text.length());
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

#endif //LIMONENGINE_HASHUTIL_H
