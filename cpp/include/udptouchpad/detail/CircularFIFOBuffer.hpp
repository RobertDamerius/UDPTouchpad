#pragma once


#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/NonCopyable.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Represents a thread-safe circular FIFO buffer.
 * @tparam T Type of a buffer element.
 * @tparam N Fixed circular buffer size.
 */
template <class T, size_t N> class CircularFIFOBuffer: public NonCopyable {
    public:
        /**
         * @brief Construct a new circular FIFO buffer.
         */
        CircularFIFOBuffer(): index(0), isFull(false){}

        /**
         * @brief Add a new value to the circular buffer.
         * @param[in] value The value to be added.
         * @details This call is thread-safe.
         */
        void Add(T value){
            std::lock_guard<std::mutex> lock(mtx);
            buffer[index] = value;
            index = (index + 1) % N;
            isFull |= !index;
        }

        /**
         * @brief Get the current FIFO buffer and clear the circular buffer.
         * @return FIFO buffer.
         * @details This call is thread-safe.
         */
        std::vector<T> Get(void){
            std::lock_guard<std::mutex> lock(mtx);
            std::vector<T> result;
            if(isFull){
                for(size_t n = 0; n < N; ++n){
                    result.push_back(buffer[index++]);
                    index %= N;
                }
            }
            else{
                if(index){
                    result.insert(result.end(), buffer.begin(), buffer.begin() + index);
                }
            }
            index = 0;
            isFull = false;
            return result;
        }

    private:
        size_t index;             // Points to the next element in the @ref buffer that can be assigned.
        bool isFull;              // True if circular buffer is full.
        std::array<T,N> buffer;   // Internal container of the circular buffer.
        std::mutex mtx;           // Protect all attributes to prevent race conditions.
};


} /* namespace: detail */


} /* namespace: udptouchpad */

