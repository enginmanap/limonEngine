//
// Created by Engin Manap on 05/11/2026.
//

#ifndef LIMONENGINE_FLATHASHTABLE_H
#define LIMONENGINE_FLATHASHTABLE_H

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

/**
 * Open addressing with linear probing, no erase. clear() only bumps the generation, a slot from an older one
 * counts as empty, so clearing every frame costs nothing and keeps the memory.
 */
template<typename Key, typename Value>
class FlatHashTable {
    static_assert(std::is_integral<Key>::value, "keys are hashed as integers");

    struct Slot {
        Key key = Key();
        uint32_t generation = 0;
        Value value = Value();
    };

    std::vector<Slot> slots;
    uint32_t generation = 1;
    uint32_t count = 0;
    uint32_t shift = 64 - 4;

    size_t homeOf(Key key) const {
        return (size_t)(((uint64_t)key * UINT64_C(0x9E3779B97F4A7C15)) >> shift);
    }

    void grow() {
        std::vector<Slot> oldSlots;
        oldSlots.swap(slots);
        slots.assign(oldSlots.size() * 2, Slot());
        --shift;
        const size_t mask = slots.size() - 1;
        for (const Slot& oldSlot : oldSlots) {
            if (oldSlot.generation != generation) {
                continue;
            }
            size_t slotIndex = homeOf(oldSlot.key);
            while (slots[slotIndex].generation == generation) {
                slotIndex = (slotIndex + 1) & mask;
            }
            slots[slotIndex] = oldSlot;
        }
    }

public:
    FlatHashTable() {
        slots.resize(16);//a power of two, homeOf takes the top bits
    }

    //nullptr when absent. Valid until the next insert
    Value* find(Key key) {
        const size_t mask = slots.size() - 1;
        for (size_t slotIndex = homeOf(key); ; slotIndex = (slotIndex + 1) & mask) {
            Slot& slot = slots[slotIndex];
            if (slot.generation != generation) {
                return nullptr;
            }
            if (slot.key == key) {
                return &slot.value;
            }
        }
    }

    //the key must be absent
    void insert(Key key, const Value& value) {
        if ((count + 1) * 2 > slots.size()) {
            grow();
        }
        const size_t mask = slots.size() - 1;
        size_t slotIndex = homeOf(key);
        while (slots[slotIndex].generation == generation) {
            slotIndex = (slotIndex + 1) & mask;
        }
        slots[slotIndex].key = key;
        slots[slotIndex].value = value;
        slots[slotIndex].generation = generation;
        ++count;
    }

    void clear() {
        ++generation;
        if (generation == 0) {
            //a slot from 4 billion clears ago would read as live
            for (Slot& slot : slots) {
                slot.generation = 0;
            }
            generation = 1;
        }
        count = 0;
    }
};

#endif //LIMONENGINE_FLATHASHTABLE_H
