#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/TouchpadPointer.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Represents the type of an event.
 */
enum EventType: uint8_t {
    event_type_error = 0,
    event_type_connection = 1,
    event_type_touchpad_pointer = 2,
    event_type_motion_sensor = 3
};


/**
 * @brief Base class for events.
 */
class EventBase {
    public:
        /**
         * @brief Construct a new event base object.
         * @param[in] t The type of the event.
         */
        explicit EventBase(const EventType t): type(t){}

        /**
         * @brief Destroy the event base object.
         */
        virtual ~EventBase(){}

        /**
         * @brief Get the type of this event.
         * @return Event type.
         */
        EventType GetType(void){
            return type;
        }

    private:
        EventType type;   // The event type set during construction.
};


} /* namespace: detail */


/**
 * @brief Represents an error event.
 */
class ErrorEvent: udptouchpad::detail::EventBase {
    public:
        std::string message;   // Error message string.

        /**
         * @brief Construct a new error event.
         */
        ErrorEvent(): udptouchpad::detail::EventBase(udptouchpad::detail::event_type_error){}

        /**
         * @brief Construct a new error event.
         * @param[in] s Error message to be set.
         */
        explicit ErrorEvent(const std::string s): udptouchpad::detail::EventBase(udptouchpad::detail::event_type_error), message(s){}

        /**
         * @brief Convert this event to a string.
         * @return String representing this event.
         */
        std::string ToString(void){
            return message;
        }
};


/**
 * @brief Represents a connection event which contains the state of connection.
 */
class DeviceConnectionEvent: udptouchpad::detail::EventBase {
    public:
        uint32_t deviceID;   // The device ID, which is equal to the IPv4 address.
        bool connected;      // True if this device has been connected (incomming data), false if it is disconnected (timeout).

        /**
         * @brief Construct a new device connection event.
         */
        DeviceConnectionEvent(): udptouchpad::detail::EventBase(udptouchpad::detail::EventType::event_type_connection), deviceID(0), connected(false) {}

        /**
         * @brief Construct a new device connection event.
         * @param[in] deviceID The device ID, which is equal to the IPv4 address.
         * @param[in] connected True if this device has been connected (incomming data), false if it is disconnected (timeout).
         */
        DeviceConnectionEvent(uint32_t deviceID, bool connected): udptouchpad::detail::EventBase(udptouchpad::detail::EventType::event_type_connection), deviceID(deviceID), connected(connected){}

        /**
         * @brief Convert this event to a string.
         * @return String representing this event.
         */
        std::string ToString(void){
            std::stringstream s;
            s << "deviceID=" << deviceID << " connected=" << static_cast<int>(connected);
            return s.str();
        }
};


/**
 * @brief Represents a touchpad pointer event.
 */
class TouchpadPointerEvent: udptouchpad::detail::EventBase {
    public:
        uint32_t deviceID;                         // The device ID, which is equal to the IPv4 address.
        double aspectRatio;                        // Aspect ratio of the touch screen, given as width/height.
        std::array<TouchpadPointer, 10> pointer;   // List of touch pointers.

        /**
         * @brief Construct a new touchpad pointer event.
         */
        TouchpadPointerEvent(): udptouchpad::detail::EventBase(udptouchpad::detail::event_type_touchpad_pointer), deviceID(0), aspectRatio(0.0) {}

        /**
         * @brief Convert this event to a string.
         * @return String representing this event.
         */
        std::string ToString(void){
            std::stringstream s;
            s << "deviceID=" << deviceID << " aspectRatio=" << aspectRatio << " pointer.pressed={" << int(pointer[0].pressed);
            for(size_t i = 1; i < pointer.size(); ++i){
                s << "," << int(pointer[i].pressed);
            }
            s << "} pointer.startPosition={";
            for(auto&& p : pointer){
                s << "{" << p.startPosition[0] << "," << p.startPosition[1] << "}";
            }
            s << "} pointer.position={";
            for(auto&& p : pointer){
                s << "{" << p.position[0] << "," << p.position[1] << "}";
            }
            s << "}";
            return s.str();
        }

        /**
         * @brief Check whether this event is equal to another event.
         * @param[in] e The event with which to compare equality.
         * @return True is e is equal to this, false otherwise.
         */
        bool IsEqual(const TouchpadPointerEvent& e) const {
            bool result = (deviceID == e.deviceID) && (aspectRatio == e.aspectRatio);
            for(size_t i = 0; i < pointer.size(); ++i){
                result &= pointer[i].IsEqual(e.pointer[i]);
            }
            return result;
        }
};


/**
 * @brief Represents the motion sensor event.
 */
class MotionSensorEvent: udptouchpad::detail::EventBase {
    public:
        uint32_t deviceID;                    // The device ID, which is equal to the IPv4 address.
        std::array<float,3> rotationVector;   // Latest 3D rotation vector sensor data from an onboard motion sensor. If no motion sensor is available, all three values are NaN.
        std::array<float,3> acceleration;     // Latest 3D accelerometer sensor data from an onboard motion sensor in m/s^2. If no motion sensor is available, all three values are NaN.
        std::array<float,3> angularRate;      // Latest 3D gyroscope sensor data from an onboard motion sensor in rad/s. If no motion sensor is available, all three values are NaN.

        /**
         * @brief Construct a new motion sensor event.
         */
        MotionSensorEvent(): udptouchpad::detail::EventBase(udptouchpad::detail::event_type_motion_sensor), deviceID(0) {
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
            s << "deviceID=" << deviceID;
            s << " rotationVector={" << rotationVector[0] << "," << rotationVector[1] << "," << rotationVector[2];
            s << "} acceleration={" << acceleration[0] << "," << acceleration[1] << "," << acceleration[2];
            s << "} angularRate={" << angularRate[0] << "," << angularRate[1] << "," << angularRate[2] << "}";
            return s.str();
        }

        /**
         * @brief Check whether this event is equal to another event.
         * @param[in] e The event with which to compare equality.
         * @return True is e is equal to this, false otherwise.
         */
        bool IsEqual(const MotionSensorEvent& e) const {
            return (deviceID == e.deviceID) &&
                   (rotationVector[0] == e.rotationVector[0]) && (rotationVector[1] == e.rotationVector[1]) && (rotationVector[2] == e.rotationVector[2]) &&
                   (acceleration[0] == e.acceleration[0]) && (acceleration[1] == e.acceleration[1]) && (acceleration[2] == e.acceleration[2]) &&
                   (angularRate[0] == e.angularRate[0]) && (angularRate[1] == e.angularRate[1]) && (angularRate[2] == e.angularRate[2]);
        }
};


} /* namespace: udptouchpad */

