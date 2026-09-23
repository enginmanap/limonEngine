//
// Created by engin on 22.09.2026.
//

#ifndef LIMONENGINE_ALIGNEDALLOCATOR_HPP
#define LIMONENGINE_ALIGNEDALLOCATOR_HPP

#include <cstddef>
#include <cstdlib>
#include <new>

/**
 * Allocator for containers whose data a SIMD reader loads directly, like SDOC's baked occluders.
 * std::vector only promises max_align_t, which is 16 bytes.
 */
template <class T, std::size_t ALIGNMENT>
class AlignedAllocator {
public:
    typedef T value_type;

    AlignedAllocator() = default;

    template <class U>
    AlignedAllocator(const AlignedAllocator<U, ALIGNMENT> &) {}

    template <class U>
    struct rebind {
        typedef AlignedAllocator<U, ALIGNMENT> other;
    };

    T *allocate(std::size_t count) {
        if (count == 0) {
            return nullptr;
        }
        std::size_t byteCount = count * sizeof(T);
        //both of these want a multiple of the alignment, the C++17 one is undefined without it
        byteCount = ((byteCount + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
#ifdef _WIN32
        void *memory = _aligned_malloc(byteCount, ALIGNMENT);
#else
        void *memory = std::aligned_alloc(ALIGNMENT, byteCount);
#endif
        if (memory == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T *>(memory);
    }

    void deallocate(T *memory, std::size_t) {
#ifdef _WIN32
        _aligned_free(memory);
#else
        std::free(memory);
#endif
    }
};

template <class T, class U, std::size_t ALIGNMENT>
bool operator==(const AlignedAllocator<T, ALIGNMENT> &, const AlignedAllocator<U, ALIGNMENT> &) {
    return true;
}

template <class T, class U, std::size_t ALIGNMENT>
bool operator!=(const AlignedAllocator<T, ALIGNMENT> &, const AlignedAllocator<U, ALIGNMENT> &) {
    return false;
}

#endif //LIMONENGINE_ALIGNEDALLOCATOR_HPP
