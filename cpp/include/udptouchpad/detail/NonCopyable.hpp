#pragma once


#include <udptouchpad/detail/Common.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Inherit from this class to make it non-copyable.
 */
class NonCopyable {
    protected:
        constexpr NonCopyable() = default;
        ~NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
};


} /* namespace: detail */


} /* namespace: udptouchpad */

