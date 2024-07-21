#pragma once


#include <udptouchpad/detail/Common.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief This union defines the constant-size touchpad message that is sent from a UDP rouchpad app.
 */
#pragma pack(push, 1)
union SerializationTouchpadMessageUnion {
    struct SerializationTouchpadMessageStruct {
        uint8_t header;                                       // Must be 0x42.
        uint8_t counter;                                      // Counter that is incremented with each new message sent.
        uint32_t screenWidth;                                 // Width of the device screen in pixels.
        uint32_t screenHeight;                                // Height of the device screen in pixels.
        std::array<uint8_t,10> pointerID;                     // ID of the pointer that touches the screen or 0xFF if a pointer is not present.
        std::array<std::array<float,2>,10> pointerPosition;   // 2D position for each pointer in pixels or zero if the corresponding pointer is not present.
        std::array<float,3> rotationVector;                   // Latest 3D rotation vector sensor data from an onboard motion sensor. If no motion sensor is available, all three values are NaN.
        std::array<float,3> acceleration;                     // Latest 3D accelerometer sensor data from an onboard motion sensor in m/s^2. If no motion sensor is available, all three values are NaN.
        std::array<float,3> angularRate;                      // Latest 3D gyroscope sensor data from an onboard motion sensor in rad/s. If no motion sensor is available, all three values are NaN.
    } data;
    uint8_t bytes[sizeof(udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct)];
};
#pragma pack(pop)


/**
 * @brief Helper function to swap endianness.
 * @tparam T Template datatype.
 * @param[in] u Value for which to swap endianness.
 * @return Input value with endianness being swapped.
 */
template <typename T> inline T SwapEndian(T t){
    static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
    union {
        T t;
        uint8_t u8[sizeof(T)];
    } source, dest;
    source.t = t;
    for(size_t k = 0; k < sizeof(T); ++k){
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    }
    return dest.t;
}


/**
 * @brief Swap the network byte order of a received touchpad message to the native host byte order.
 * @param[inout] msg The touchpad message for which to swap the byte order.
 */
inline void NetworkToNativeByteOrder(SerializationTouchpadMessageUnion& msg){
    if constexpr (std::endian::native != std::endian::big){
        msg.data.screenWidth = SwapEndian(msg.data.screenWidth);
        msg.data.screenHeight = SwapEndian(msg.data.screenHeight);
        for(auto&& p : msg.data.pointerPosition){
            for(auto&& f : p){
                f = SwapEndian(f);
            }
        }
        for(auto&& f : msg.data.rotationVector){
            f = SwapEndian(f);
        }
        for(auto&& f : msg.data.acceleration){
            f = SwapEndian(f);
        }
        for(auto&& f : msg.data.angularRate){
            f = SwapEndian(f);
        }
    }
}


} /* namespace: detail */


} /* namespace: udptouchpad */

