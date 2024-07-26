#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>
#include <udptouchpad/detail/TouchpadPointer.hpp>
#include <udptouchpad/detail/Events.hpp>
#include <udptouchpad/detail/DeviceData.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Stores all data for motion sensor and touchpad pointer events for all devices.
 */
class DeviceDatabase {
    public:
        /**
         * @brief Push a new message, received from a specific device, to the database.
         * @param[in] deviceID ID of the device, e.g. the source address of the message.
         * @param[in] msg The message that has been received from the specified device.
         * @details This function is thread-safe.
         */
        void PushNewMessage(const uint32_t deviceID, const udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct& msg){
            std::lock_guard<std::mutex> lock(mtx);
            auto it = devices.find(deviceID);
            if(it != devices.end()){ // deviceID exists
                udptouchpad::detail::DeviceData& device = it->second;

                // discard message if counter is too old
                bool counterIsOld = ((static_cast<int32_t>(msg.counter) + 255 - static_cast<int32_t>(device.messageCounter)) % 256) >= 127;
                if(counterIsOld) return;

                // generate new data
                udptouchpad::detail::DeviceData newData = GenerateDefaultDeviceData(msg);

                // set start position for pointers that are pressed the first time
                for(size_t i = 0; i < newData.pointer.size(); ++i){
                    newData.pointer[i].startPosition = device.pointer[i].startPosition; // keep start position of previous event
                    if(newData.pointer[i].pressed && !device.pointer[i].pressed){ // update start position if pointer is pressed
                        newData.pointer[i].startPosition = newData.pointer[i].position;
                    }
                    if(!newData.pointer[i].pressed){ // keep previous pointer position if a new pointer is not pressed
                        newData.pointer[i].position = device.pointer[i].position;
                    }
                }

                // check if new data is available
                auto [newPointerData, newMotionData] = DetectDataChange(newData, device);
                newData.newMotionDataAvailable = device.newMotionDataAvailable || (newMotionData && MotionSensorDataIsFinite(newData));

                // update device data and add events
                device = newData;
                if(newPointerData){
                    events.push_back(reinterpret_cast<udptouchpad::detail::EventBase*>(device.NewTouchpadPointerEvent(deviceID)));
                }
            }
            else{ // deviceID does not exist
                udptouchpad::detail::DeviceData newData = GenerateDefaultDeviceData(msg);
                devices.insert(std::make_pair(deviceID, newData));
                events.push_back(reinterpret_cast<udptouchpad::detail::EventBase*>(new udptouchpad::DeviceConnectionEvent(deviceID, true)));
            }
        }

        /**
         * @brief Fetch new events from the device database and run callback functions.
         * @param[in] fDeviceConnection The function to be called for device connection events.
         * @param[in] fTouchpadPointer The function to be called for touchpad pointer events.
         * @param[in] fMotionSensor The function to be called for motion sensor events.
         * @details This function is thread-safe.
         */
        void FetchEvents(std::function<void(udptouchpad::DeviceConnectionEvent)> fDeviceConnection, std::function<void(udptouchpad::TouchpadPointerEvent)> fTouchpadPointer, std::function<void(udptouchpad::MotionSensorEvent)> fMotionSensor){
            std::lock_guard<std::mutex> lock(mtx);

            // fetch all connection and touchpad events, insert them to output and delete them from the internal events container
            for(auto&& e : events){
                switch(e->GetType()){
                    case udptouchpad::detail::event_type_connection:
                        if(fDeviceConnection){
                            fDeviceConnection(*reinterpret_cast<udptouchpad::DeviceConnectionEvent*>(e));
                        }
                        break;
                    case udptouchpad::detail::event_type_touchpad_pointer:
                        if(fTouchpadPointer){
                            fTouchpadPointer(*reinterpret_cast<udptouchpad::TouchpadPointerEvent*>(e));
                        }
                        break;
                    case udptouchpad::detail::event_type_motion_sensor:
                        // is not fired here
                        break;
                    case udptouchpad::detail::event_type_error:
                        // is not fired here
                        break;
                }
                delete e;
            }
            events.clear();

            // check connection status and fetch new motion sensor events
            for(auto it = devices.begin(); it != devices.end();){
                if(it->second.TimeToLatestReceivedMessage() > UDP_TOUCHPAD_DEVICE_DISCONNECT_TIMEOUT_S){
                    if(fDeviceConnection){
                        fDeviceConnection(udptouchpad::DeviceConnectionEvent(it->first, false));
                    }
                    it = devices.erase(it);
                }
                else{
                    if(it->second.newMotionDataAvailable){
                        it->second.newMotionDataAvailable = false;
                        if(fMotionSensor){
                            fMotionSensor(it->second.ToMotionSensorEvent(it->first));
                        }
                    }
                    it++;
                }
            }
        }

    private:
        std::unordered_map<uint32_t, udptouchpad::detail::DeviceData> devices;   // Internal data storage for all devices.
        std::vector<udptouchpad::detail::EventBase*> events;                     // Stores connection and touchpad pointer events.
        std::mutex mtx;                                                          // Protect @ref devices and @ref events.

        /**
         * @brief Generate default device data based on a received message.
         * @param[in] msg The message from which to generate the device data.
         * @return Generated device data, where the timestamp is set to the current time and the start position for all pointers is equal to their position.
         */
        udptouchpad::detail::DeviceData GenerateDefaultDeviceData(const udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct& msg){
            udptouchpad::detail::DeviceData result;
            result.messageCounter = msg.counter;
            result.timestampReceive = std::chrono::steady_clock::now();
            result.newMotionDataAvailable = false;
            result.rotationVector = msg.rotationVector;
            result.acceleration = msg.acceleration;
            result.angularRate = msg.angularRate;
            result.aspectRatio = static_cast<double>(msg.screenWidth) / static_cast<double>(msg.screenHeight);
            for(size_t i = 0; (i < msg.pointerID.size()) && (i < msg.pointerPosition.size()); ++i){
                if(msg.pointerID[i] >= 10) continue;
                result.pointer[msg.pointerID[i]].pressed = true;
                result.pointer[msg.pointerID[i]].position[0] = static_cast<double>(msg.pointerPosition[i][0]) / static_cast<double>(msg.screenWidth);
                result.pointer[msg.pointerID[i]].position[1] = static_cast<double>(msg.pointerPosition[i][1]) / static_cast<double>(msg.screenHeight);
                result.pointer[msg.pointerID[i]].startPosition = result.pointer[msg.pointerID[i]].position;
            }
            return result;
        }

        /**
         * @brief Check whether there are changes in touchpad pointer and motion sensor data values.
         * @param[in] a First device data to compare.
         * @param[in] b Second device data to compare.
         * @return Two flags indicating if touchpad pointer data (<0>) and/or motion sensor data (<1>) is different.
         */
        std::tuple<bool, bool> DetectDataChange(const udptouchpad::detail::DeviceData& a, const udptouchpad::detail::DeviceData& b){
            bool pointerDataChanged = (a.aspectRatio != b.aspectRatio);
            for(size_t i = 0; i < a.pointer.size(); ++i){
                pointerDataChanged |= !a.pointer[i].IsEqual(b.pointer[i]);
            }
            bool motionDataChanged = (a.rotationVector[0] != b.rotationVector[0]) || (a.rotationVector[1] != b.rotationVector[1]) || (a.rotationVector[2] != b.rotationVector[2]);
            motionDataChanged |= (a.acceleration[0] != b.acceleration[0]) || (a.acceleration[1] != b.acceleration[1]) || (a.acceleration[2] != b.acceleration[2]);
            motionDataChanged |= (a.angularRate[0] != b.angularRate[0]) || (a.angularRate[1] != b.angularRate[1]) || (a.angularRate[2] != b.angularRate[2]);
            return std::make_tuple(pointerDataChanged, motionDataChanged);
        }

        /**
         * @brief Check whether motion sensor data is finite.
         * @param[in] d Device data to be checked.
         * @return True if motion sensor data (all values) of the specified device data is finite, false otherwise.
         */
        bool MotionSensorDataIsFinite(const udptouchpad::detail::DeviceData& d){
            bool isFinite = std::isfinite(d.rotationVector[0]) && std::isfinite(d.rotationVector[1]) && std::isfinite(d.rotationVector[2]);
            isFinite &= std::isfinite(d.acceleration[0]) && std::isfinite(d.acceleration[1]) && std::isfinite(d.acceleration[2]);
            isFinite &= std::isfinite(d.angularRate[0]) && std::isfinite(d.angularRate[1]) && std::isfinite(d.angularRate[2]);
            return isFinite;
        }
};


} /* namespace: detail */


} /* namespace: udptouchpad */

