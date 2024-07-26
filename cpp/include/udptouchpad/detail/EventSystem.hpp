#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/EventSystemNetworkBase.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>
#include <udptouchpad/detail/CircularFIFOBuffer.hpp>
#include <udptouchpad/detail/Events.hpp>
#include <udptouchpad/detail/DeviceDatabase.hpp>


namespace udptouchpad {


/**
 * @brief Processes received messages from UDP touchpad apps and stores them in a database. The events
 * can be polled from a user thread to run specified callback functions.
 */
class EventSystem: public udptouchpad::detail::EventSystemNetworkBase {
    public:
        /**
         * @brief Set callback function for error events.
         * @param[in] f The callback function with prototype void(udptouchpad::ErrorEvent).
         */
        void SetErrorCallback(std::function<void(udptouchpad::ErrorEvent)> f){
            callbackError = f;
        }

        /**
         * @brief Set callback function for device connection events.
         * @param[in] f The callback function with prototype void(udptouchpad::DeviceConnectionEvent).
         */
        void SetDeviceConnectionCallback(std::function<void(udptouchpad::DeviceConnectionEvent)> f){
            callbackDeviceConnection = f;
        }

        /**
         * @brief Set callback function for touchpad pointer events.
         * @param[in] f The callback function with prototype void(udptouchpad::TouchpadPointerEvent).
         */
        void SetTouchpadPointerCallback(std::function<void(udptouchpad::TouchpadPointerEvent)> f){
            callbackTouchpadPointer = f;
        }

        /**
         * @brief Set callback function for motion sensor events.
         * @param[in] f The callback function with prototype void(udptouchpad::MotionSensorEvent).
         */
        void SetMotionSensorCallback(std::function<void(udptouchpad::MotionSensorEvent)> f){
            callbackMotionSensor = f;
        }

        /**
         * @brief Poll events and run user-defined callback functions.
         */
        void PollEvents(void){
            auto errorEvents = errorBuffer.Get();
            if(callbackError){
                for(auto&& e : errorEvents){
                    callbackError(e);
                }
            }
            deviceDatabase.FetchEvents(callbackDeviceConnection, callbackTouchpadPointer, callbackMotionSensor);
        }

    protected:
        /**
         * @brief Process an error message.
         * @param[in] msg The error message to be handled.
         */
        void ProcessErrorMessage(const std::string& msg){
            errorBuffer.Add(ErrorEvent(msg));
        }

        /**
         * @brief Process a received message from the UDP touchpad app.
         * @param[in] source The source address from where the message was sent.
         * @param[in] msg The message that has been received.
         */
        void ProcessTouchMessage(const uint32_t source, const udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct& msg){
            deviceDatabase.PushNewMessage(source, msg);
        }

    private:
        /* user-defined callbacks */
        std::function<void(udptouchpad::ErrorEvent)> callbackError;                         // Callback for error messages.
        std::function<void(udptouchpad::DeviceConnectionEvent)> callbackDeviceConnection;   // Callback for device connection events.
        std::function<void(udptouchpad::TouchpadPointerEvent)> callbackTouchpadPointer;     // Callback for touchpad pointer events.
        std::function<void(udptouchpad::MotionSensorEvent)> callbackMotionSensor;           // Callback for motion sensor events.

        /* event buffers */
        udptouchpad::detail::CircularFIFOBuffer<udptouchpad::ErrorEvent, 64> errorBuffer;   // Thread-safe buffer for error messages.
        udptouchpad::detail::DeviceDatabase deviceDatabase;                                 // Stores data for touchpad pointer and motion sensor events.
};


} /* namespace: udptouchpad */

