#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>
#include <udptouchpad/detail/TouchpadPointer.hpp>
#include <udptouchpad/detail/Events.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Represent a data entry of the @ref DeviceDatabase.
 */
class DeviceData {
    public:
        /* general */
        uint8_t messageCounter;                                                // Message counter of latest message that has been received from this device.
        std::chrono::time_point<std::chrono::steady_clock> timestampReceive;   // Timepoint when latest message has been received from this device.

        /* motion sensor data */
        bool newMotionDataAvailable;          // True if new motion data is available, false otherwise.
        std::array<float,3> rotationVector;   // Latest 3D rotation vector sensor data from an onboard motion sensor. If no motion sensor is available, all three values are NaN.
        std::array<float,3> acceleration;     // Latest 3D accelerometer sensor data from an onboard motion sensor in m/s^2. If no motion sensor is available, all three values are NaN.
        std::array<float,3> angularRate;      // Latest 3D gyroscope sensor data from an onboard motion sensor in rad/s. If no motion sensor is available, all three values are NaN.

        /* pointer data */
        double aspectRatio;                                     // Aspect ratio of the touch screen, given as width/height.
        std::array<udptouchpad::TouchpadPointer, 10> pointer;   // List of touch pointers.

        /**
         * @brief Construct a new device data object.
         */
        DeviceData(): messageCounter(0) {
            rotationVector.fill(std::numeric_limits<float>::quiet_NaN());
            acceleration.fill(std::numeric_limits<float>::quiet_NaN());
            angularRate.fill(std::numeric_limits<float>::quiet_NaN());
            aspectRatio = 0.0;
            newMotionDataAvailable = false;
        }

        /**
         * @brief Measure the elapsed time to the @ref timestampReceive timepoint.
         * @return Time (seconds) to the latest received message.
         */
        double TimeToLatestReceivedMessage(void){
            auto timepointNow = std::chrono::steady_clock::now();
            return 1.0e-9 * static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(timepointNow - timestampReceive).count());
        }

        /**
         * @brief Convert this data to a touchpad pointer event.
         * @param[in] deviceID The device ID to be set for the event.
         * @return A touchpad pointer event built from this device data.
         */
        udptouchpad::TouchpadPointerEvent ToTouchpadPointerEvent(const uint32_t deviceID){
            udptouchpad::TouchpadPointerEvent event;
            event.deviceID = deviceID;
            event.aspectRatio = aspectRatio;
            event.pointer = pointer;
            return event;
        }

        /**
         * @brief Convert this data to a motion sensor event.
         * @param[in] deviceID The device ID to be set for the event.
         * @return A motion sensor event built from this device data.
         */
        udptouchpad::MotionSensorEvent ToMotionSensorEvent(const uint32_t deviceID){
            udptouchpad::MotionSensorEvent event;
            event.deviceID = deviceID;
            event.rotationVector = rotationVector;
            event.acceleration = acceleration;
            event.angularRate = angularRate;
            return event;
        }

        /**
         * @brief Create a new touchpad pointer event based on this data.
         * @param[in] deviceID The device ID to be set for the event.
         * @return A touchpad pointer event built from this device data.
         */
        udptouchpad::TouchpadPointerEvent* NewTouchpadPointerEvent(const uint32_t deviceID){
            udptouchpad::TouchpadPointerEvent* event = new udptouchpad::TouchpadPointerEvent();
            event->deviceID = deviceID;
            event->aspectRatio = aspectRatio;
            event->pointer = pointer;
            return event;
        }

        /**
         * @brief Create a new motion sensor eventbased on this data.
         * @param[in] deviceID The device ID to be set for the event.
         * @return A motion sensor event built from this device data.
         */
        udptouchpad::MotionSensorEvent* NewMotionSensorEvent(const uint32_t deviceID){
            udptouchpad::MotionSensorEvent* event = new udptouchpad::MotionSensorEvent();
            event->deviceID = deviceID;
            event->rotationVector = rotationVector;
            event->acceleration = acceleration;
            event->angularRate = angularRate;
            return event;
        }
};


} /* namespace: detail */


} /* namespace: udptouchpad */

