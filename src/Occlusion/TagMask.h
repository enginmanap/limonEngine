#ifndef LIMONENGINE_TAGMASK_H
#define LIMONENGINE_TAGMASK_H

#include <cstdint>

//one bit per object tag the world's pipeline draws, RenderTagBits gives the numbers. A tag no stage draws has no bit
struct TagMask {
    uint64_t low = 0;
    uint64_t high = 0;

    void set(uint32_t bit) {
        if (bit < 64) {
            low |= UINT64_C(1) << bit;
        } else {
            high |= UINT64_C(1) << (bit - 64);
        }
    }

    bool intersects(const TagMask &other) const {
        return ((low & other.low) | (high & other.high)) != 0;
    }
};

#endif //LIMONENGINE_TAGMASK_H
