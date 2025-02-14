#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/NonCopyable.hpp>
#include <udptouchpad/detail/MulticastSocket.hpp>
#include <udptouchpad/detail/ConditionVariable.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Abstract base class for the event system that handles all network stuff.
 */
class EventSystemNetworkBase: public udptouchpad::detail::NonCopyable {
    public:
        /**
         * @brief Construct a new event system base.
         */
        EventSystemNetworkBase(){
            terminate = false;
            workerThread = std::thread(&EventSystemNetworkBase::ThreadFunction, this);
        }

        /**
         * @brief Destroy the event system base.
         */
        virtual ~EventSystemNetworkBase(){
            terminate = true;
            udpSocket.Close();
            retryTimer.NotifyOne();
            if(workerThread.joinable()){
                workerThread.join();
            }
        }

    protected:
        /**
         * @brief Process an error message.
         * @param[in] msg The error message to be handled.
         */
        virtual void ProcessErrorMessage(const std::string& msg) = 0;

        /**
         * @brief Process a received touch message.
         * @param[in] source The source address from where the message was sent.
         * @param[in] msg The touch message that has been received.
         */
        virtual void ProcessTouchMessage(const uint32_t source, const udptouchpad::detail::SerializationTouchpadMessageUnion::SerializationTouchpadMessageStruct& msg) = 0;

    private:
        std::atomic<bool> terminate;                         // Flag that indicates, whether the worker thread should be terminated or not.
        std::thread workerThread;                            // Thread object for the worker thread.
        udptouchpad::detail::MulticastSocket udpSocket;      // Multicast UDP socket.
        udptouchpad::detail::ConditionVariable retryTimer;   // A timer to wait before retrying to open a UDP socket in case of errors.

        /**
         * @brief The worker thread function.
         */
        void ThreadFunction(void){
            // local buffer where to store received messages
            constexpr size_t rxBufferSize = 65507;
            uint8_t* rxBuffer = new uint8_t[rxBufferSize];

            std::string previousErrorString;
            while(!terminate){
                // (re)-open socket
                if(!udpSocket.Open()){
                    std::string errorString = udpSocket.GetErrorString();
                    if(errorString.compare(previousErrorString)){
                        previousErrorString = errorString;
                        ProcessErrorMessage(errorString);
                    }
                    retryTimer.WaitFor(UDP_TOUCHPAD_REOPEN_SOCKET_RETRY_TIME_MS);
                    continue;
                }

                // receive and unpack
                while(!terminate && udpSocket.IsOpen()){
                    uint32_t source;
                    auto [rx, errorCode] = udpSocket.ReceiveFrom(source, &rxBuffer[0], rxBufferSize);
                    if(!udpSocket.IsOpen() || terminate){
                        break;
                    }
                    if(rx < 0){
                        #ifdef _WIN32
                        if(WSAEMSGSIZE == errorCode){
                            continue;
                        }
                        #endif
                        retryTimer.WaitFor(UDP_TOUCHPAD_REOPEN_SOCKET_RETRY_TIME_MS);
                        break;
                    }
                    UnpackMessage(source, &rxBuffer[0], rx);
                    std::this_thread::yield();
                }

                // terminate the socket
                udpSocket.Close();
            }
            delete[] rxBuffer;
        }

        /**
         * @brief Process a received UDP message.
         * @param[in] source The source from which the message was sent.
         * @param[in] bytes The bytes containing the message.
         * @param[in] length The length of the received UDP message.
         */
        void UnpackMessage(uint32_t source, uint8_t* bytes, int32_t length){
            if(length == sizeof(udptouchpad::detail::SerializationTouchpadMessageUnion)){
                udptouchpad::detail::SerializationTouchpadMessageUnion* msg = reinterpret_cast<udptouchpad::detail::SerializationTouchpadMessageUnion*>(bytes);
                if(0x42 == msg->data.header){
                    udptouchpad::detail::NetworkToNativeByteOrder(*msg);
                    ProcessTouchMessage(source, msg->data);
                }
            }
        }
};


} /* namespace: detail */


} /* namespace: udptouchpad */

