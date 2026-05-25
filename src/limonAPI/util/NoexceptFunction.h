//
// Created by Engin Manap on 25/05/2026.
//

#ifndef LIMONENGINE_NOEXCEPTFUNCTION_H
#define LIMONENGINE_NOEXCEPTFUNCTION_H

#include <functional>
#include <type_traits>

template <typename Signature>
class NoexceptFunction;

template <typename Ret, typename... Args>
class NoexceptFunction<Ret(Args...)> {
    std::function<Ret(Args...)> internal_func;

public:
    // 1. Constructors
    NoexceptFunction() : internal_func(nullptr) {}
    NoexceptFunction(std::nullptr_t) : internal_func(nullptr) {}

    NoexceptFunction(const NoexceptFunction&) = default;
    NoexceptFunction(NoexceptFunction&&) noexcept = default;

    // 2. The Magic Constructor for Callables
    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, NoexceptFunction>>>
    NoexceptFunction(F&& func) {
        static_assert(noexcept(std::declval<F>()(std::declval<Args>()...)),
                      "ERROR: You can only assign 'noexcept' callables to this function wrapper!");

        internal_func = std::forward<F>(func);
    }

    // 3. Assignment Operators
    NoexceptFunction& operator=(const NoexceptFunction&) = default;
    NoexceptFunction& operator=(NoexceptFunction&&) noexcept = default;

    NoexceptFunction& operator=(std::nullptr_t) noexcept {
        internal_func = nullptr;
        return *this;
    }

    // 4. Execution & Bool check
    Ret operator()(Args... args) const noexcept {
        if (internal_func) {
            return internal_func(std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(internal_func);
    }
};


#endif //LIMONENGINE_NOEXCEPTFUNCTION_H
