//
// Created by Engin Manap on 03/08/2022.
//

#ifndef LIMONENGINE_STRINGID_H
#define LIMONENGINE_STRINGID_H


#include <cstdint>
#include <string>
#include "cityhash/src/city.h"
#include <unordered_map>

class SID {
public:
    static std::unordered_map<uint32_t, std::string> stringIdMap;

    static constexpr uint32_t hash(const std::string &string) {
        uint32_t hash = CityHash32(string.c_str(), string.length());
        stringIdMap[hash] = string;
        return hash;
    }
};

#endif //LIMONENGINE_STRINGID_H
