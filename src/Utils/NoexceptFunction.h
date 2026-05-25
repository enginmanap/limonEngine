//
// Created by Engin Manap on 25/05/2026.
//

#ifndef LIMONENGINE_NOEXCEPTFUNCTION_H
#define LIMONENGINE_NOEXCEPTFUNCTION_H

#include <functional>

class NoexceptFunction {
    std::function<void()> internal_func;

public:
    // 1. Constructors
    NoexceptFunction() : internal_func(nullptr) {}
    NoexceptFunction(std::nullptr_t) : internal_func(nullptr) {}

    // Default copy/move constructors so it behaves like a standard object
    NoexceptFunction(const NoexceptFunction&) = default;
    NoexceptFunction(NoexceptFunction&&) noexcept = default;

    // 2. The Magic Constructor for Callables
    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, NoexceptFunction>>>
    NoexceptFunction(F&& func) {
        static_assert(noexcept(std::declval<F>()()),
                      "ERROR: You can only assign 'noexcept' callables to this function wrapper!");

        internal_func = std::forward<F>(func);
    }

    // 3. Assignment Operators (Fixes the nullptr initialization/assignment issue)
    NoexceptFunction& operator=(const NoexceptFunction&) = default;
    NoexceptFunction& operator=(NoexceptFunction&&) noexcept = default;

    NoexceptFunction& operator=(std::nullptr_t) noexcept {
        internal_func = nullptr;
        return *this;
    }

    // 4. Execution & Bool check
    void operator()() const noexcept {
        if (internal_func) {
            internal_func();
        }
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(internal_func);
    }
};


#endif //LIMONENGINE_NOEXCEPTFUNCTION_H