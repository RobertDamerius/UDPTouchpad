#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/EventSystemNetworkBase.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>
#include <udptouchpad/detail/CircularFIFOBuffer.hpp>
#include <udptouchpad/detail/Events.hpp>


namespace udptouchpad {


template <size_t BUFSIZE_ERROR_EVENTS, size_t BUFSIZE_TOUCHPAD_EVENTS> class EventSystem: public udptouchpad::detail::EventSystemNetworkBase {
    public:
        /**
         * @brief Set callback function for error events.
         * @param[in] f The callback function with prototype void(udptouchpad::ErrorEvent).
         */
        void SetErrorCallback(std::function<void(udptouchpad::ErrorEvent)> f){
            callbackError = f;
        }

        /**
         * @brief Set callback function for touchpad events.
         * @param[in] f The callback function with prototype void(udptouchpad::TouchpadEvent).
         */
        void SetTouchpadCallback(std::function<void(udptouchpad::TouchpadEvent)> f){
            callbackTouchpad = f;
        }

        /**
         * @brief Poll events and run user-defined callback functions.
         */
        void PollEvents(void){
            auto errorEvents = bufferErrorEvents.Get();
            if(callbackError){
                for(auto&& e : errorEvents){
                    callbackError(e);
                }
            }
            auto touchpadEvents = bufferTouchpadEvents.Get();
            if(callbackTouchpad){
                for(auto&& e : touchpadEvents){
                    callbackTouchpad(e);
                }
            }
        }

    protected:
        /**
         * @brief Process an error message.
         * @param[in] msg The error message to be handled.
         */
        void ProcessErrorMessage(const std::string& msg){
            bufferErrorEvents.Add(ErrorEvent(msg));
        }

        /**
         * @brief Process a received touch message.
         * @param[in] source The source address from where the message was sent.
         * @param[in] msg The touch message that has been received.
         */
        void ProcessTouchMessage(const uint32_t source, const udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct& msg){
            if(!ShouldBeDiscarded(source, msg.counter)){
                TouchpadEvent e;
                e.deviceID = source;
                e.screenWidth = msg.screenWidth;
                e.screenHeight = msg.screenWidth;
                e.pointerID = msg.pointerID;
                e.pointerPosition = msg.pointerPosition;
                e.rotationVector = msg.rotationVector;
                e.acceleration = msg.acceleration;
                e.angularRate = msg.angularRate;
                bufferTouchpadEvents.Add(e);
            }
        }

    private:
        /* User-defined callbacks */
        std::function<void(udptouchpad::ErrorEvent)> callbackError;                              // Callback for error messages.
        std::function<void(udptouchpad::TouchpadEvent)> callbackTouchpad;                        // Callback for error messages.

        /* Event buffers (circular FIFO) */
        udptouchpad::detail::CircularFIFOBuffer<udptouchpad::ErrorEvent, BUFSIZE_ERROR_EVENTS> bufferErrorEvents;          // Buffer for error messages.
        udptouchpad::detail::CircularFIFOBuffer<udptouchpad::TouchpadEvent, BUFSIZE_TOUCHPAD_EVENTS> bufferTouchpadEvents;   // Buffer for touchpad messages.

        /* Stuff to check the message counter */
        struct SourceInfo {
            uint8_t counter;                                                // Counter of last message.
            std::chrono::time_point<std::chrono::steady_clock> timepoint;   // Timepoint when last message has been received.
        };
        std::unordered_map<uint32_t, SourceInfo> sourceMap;                 // Contains past counters for all sources.


        /**
         * @brief Check if a message should be discarded based on the counter value.
         * @param[in] source The source address from where the message was sent.
         * @param[in] messageCounter Counter of the current message.
         * @return True if message should be discarded, false otherwise.
         * @details A message should be discarded if its counter "lies in the past".
         */
        bool ShouldBeDiscarded(uint32_t source, uint8_t messageCounter){
            auto timepointNow = std::chrono::steady_clock::now();
            bool discardMessage = false;
            auto got = sourceMap.find(source);
            if(got == sourceMap.end()){
                SourceInfo info;
                info.counter = messageCounter;
                info.timepoint = timepointNow;
                sourceMap.insert(std::pair<uint32_t, SourceInfo>(source, info));
            }
            else{
                double secondsToPreviousMessage = 1.0e-9 * static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(timepointNow - got->second.timepoint).count());
                bool counterIsOld = ((static_cast<int32_t>(messageCounter) + 255 - static_cast<int32_t>(got->second.counter)) % 256) >= 127;
                if((secondsToPreviousMessage < 1.0) && counterIsOld){
                    discardMessage = true;
                }
                else{
                    got->second.counter = messageCounter;
                    got->second.timepoint = timepointNow;
                }
            }
            return discardMessage;
        }
};


} /* namespace: udptouchpad */

