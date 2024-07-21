#pragma once


#include <udptouchpad/detail/Common.hpp>



namespace udptouchpad {


/**
 * @brief Represents an error event.
 */
class ErrorEvent {
    public:
        std::string message;   // Error message string.

        /**
         * @brief Construct a new error event.
         */
        ErrorEvent(){}

        /**
         * @brief Construct a new error event.
         * @param[in] s Error message to be set.
         */
        explicit ErrorEvent(const std::string s): message(s){}

        /**
         * @brief Convert this event to a string.
         * @return String representing this event.
         */
        std::string ToString(void){
            return message;
        }
};


/**
 * @brief Represents a touchpad event.
 */
class TouchpadEvent {
    public:
        uint32_t deviceID;                                    // The device ID, which is equal to the IPv4 address.
        uint32_t screenWidth;                                 // Width of the device screen in pixels.
        uint32_t screenHeight;                                // Height of the device screen in pixels.
        std::array<uint8_t,10> pointerID;                     // ID of the pointer that touches the screen or 0xFF if a pointer is not present.
        std::array<std::array<float,2>,10> pointerPosition;   // 2D position for each pointer in pixels or zero if the corresponding pointer is not present.
        std::array<float,3> rotationVector;                   // Latest 3D rotation vector sensor data from an onboard motion sensor. If no motion sensor is available, all three values are NaN.
        std::array<float,3> acceleration;                     // Latest 3D accelerometer sensor data from an onboard motion sensor in m/s^2. If no motion sensor is available, all three values are NaN.
        std::array<float,3> angularRate;                      // Latest 3D gyroscope sensor data from an onboard motion sensor in rad/s. If no motion sensor is available, all three values are NaN.

        /**
         * @brief Construct a new touchpad event.
         */
        TouchpadEvent(): deviceID(0), screenWidth(0), screenHeight(0){
            pointerID.fill(0xFF);
            pointerPosition.fill({0.0, 0.0});
            rotationVector.fill(std::numeric_limits<float>::quiet_NaN());
            acceleration.fill(std::numeric_limits<float>::quiet_NaN());
            angularRate.fill(std::numeric_limits<float>::quiet_NaN());
        }

        /**
         * @brief Convert this event to a string.
         * @return String representing this event.
         */
        std::string ToString(void){
            std::stringstream s;
            s << "deviceID=" << deviceID << " screenWidth=" << screenWidth << " screenHeight=" << screenHeight << " pointerID={" << std::to_string(pointerID[0]);
            for(size_t i = 1; i < pointerID.size(); ++i){
                s << "," << std::to_string(pointerID[i]);
            }
            s << "} pointerPosition={";
            for(auto&& p : pointerPosition){
                s << "{" << p[0] << "," << p[1] << "}";
            }
            s << "} rotationVector={" << rotationVector[0] << "," << rotationVector[1] << "," << rotationVector[2];
            s << "} acceleration={" << acceleration[0] << "," << acceleration[1] << "," << acceleration[2];
            s << "} angularRate={" << angularRate[0] << "," << angularRate[1] << "," << angularRate[2] << "}";
            return s.str();
        }
};


} /* namespace: udptouchpad */

