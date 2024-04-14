//
// Created by engin on 25/10/2023.
//

#ifndef LIMONENGINE_HASHUTIL_HPP
#define LIMONENGINE_HASHUTIL_HPP

#include <string>
#include "../../libs/cityhash/src/city.h"

class HashUtil {
public:
    struct HashedString {
        const uint64_t hash;
        const std::string text;
        HashedString(const std::string& text): hash(CityHash64(text.c_str(), text.length())), text(text){}
    };

    static uint64_t hashString(const std::string& text) {
        return CityHash64(text.c_str(), text.length());
    }


};


#endif //LIMONENGINE_HASHUTIL_HPP
