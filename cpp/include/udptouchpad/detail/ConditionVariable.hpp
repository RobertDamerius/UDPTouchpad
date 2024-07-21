#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/NonCopyable.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief The condition variable can be used to wait within a thread for an event that is notified by another thread.
 */
class ConditionVariable: public udptouchpad::detail::NonCopyable {
    public:
        /**
         * @brief Notify one thread waiting for this condition.
         */
        void NotifyOne(){
            {
                std::unique_lock<std::mutex> lock(mtx);
                notified = true;
            }
            cv.notify_one();
        }

        /**
         * @brief Wait for a notification. A thread calling this function waits until @ref NotifyOne is called.
         */
        void Wait(void){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this](){ return this->notified; });
            notified = false;
        }

        /**
         * @brief Wait for a notification or for a timeout. A thread calling this function waits until @ref NotifyOne is called or the wait timed out.
         * @param[in] timeoutMs Timeout in milliseconds.
         */
        void WaitFor(uint32_t timeoutMs){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this](){ return this->notified; });
            notified = false;
        }

        /**
         * @brief Clear a notified state.
         */
        void Clear(void){
            std::unique_lock<std::mutex> lock(mtx);
            notified = false;
        }

    protected:
        std::mutex mtx;               // The mutex used for event notification.
        std::condition_variable cv;   // The condition variable used for event notification.
        bool notified;                // A boolean flag to prevent spurious wakeups.
};


} /* namespace: detail */


} /* namespace: udptouchpad */

