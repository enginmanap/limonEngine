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


//the same CityHash64 as HASH(), so values match. consthash recurses once per 64 bytes, which takes minutes on mesh sized data
inline uint64_t hash(const void *data, size_t length) {
    using namespace consthash::__detail;
    const char *bytes = static_cast<const char *>(data);
    if (length <= 64) {
        return consthash::city64(bytes, length);
    }
    uint64_t x = Fetch64(bytes + length - 40);
    uint64_t y = Fetch64(bytes + length - 16) + Fetch64(bytes + length - 56);
    uint64_t z = HashLen16(Fetch64(bytes + length - 48) + length, Fetch64(bytes + length - 24));
    uint128_t v = WeakHashLen32WithSeeds(bytes + length - 64, length, z);
    uint128_t w = WeakHashLen32WithSeeds(bytes + length - 32, y + k1, x);
    x = x * k1 + Fetch64(bytes);
    size_t remaining = (length - 1) & ~static_cast<size_t>(63);
    while (remaining != 0) {
        uint64_t nextX = (Rotate(x + y + v.first + Fetch64(bytes + 8), 37) * k1) ^ w.second;
        uint64_t nextY = (Rotate(y + v.second + Fetch64(bytes + 48), 42) * k1) + v.first + Fetch64(bytes + 40);
        uint64_t nextZ = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(bytes, v.second * k1, nextX + w.first);
        w = WeakHashLen32WithSeeds(bytes + 32, nextZ + w.second, nextY + Fetch64(bytes + 16));
        x = nextZ;//x and z trade places every block, as in the reference
        y = nextY;
        z = nextX;
        bytes += 64;
        remaining -= 64;
    }
    return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z, HashLen16(v.second, w.second) + x);
}

inline uint64_t hash(const std::string &text) {
    return hash(text.data(), text.length());
}

#endif //HASHUTIL_H
