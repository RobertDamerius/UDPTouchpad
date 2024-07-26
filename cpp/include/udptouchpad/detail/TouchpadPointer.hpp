#pragma once


#include <udptouchpad/detail/Common.hpp>


namespace udptouchpad {


/**
 * @brief Represents the data of one touchpad pointer in a touchpad pointer event.
 */
class TouchpadPointer {
    public:
        bool pressed;                         // True if this pointer is pressed, false otherwise.
        std::array<double,2> startPosition;   // Pointer position when this pointer was pressed in relative device screen coordinates in [0,1].
        std::array<double,2> position;        // Current pointer position in relative device screen coordinates in [0,1].

        /**
         * @brief Construct a new touchpad pointer.
         */
        TouchpadPointer(): pressed(false) {
            startPosition.fill(0.0);
            position.fill(0.0);
        }

        /**
         * @brief Check whether this data is equal to another touchpad pointer.
         * @param[in] p The touchpad pointer with which to compare equality.
         * @return True is p is equal to this, false otherwise.
         */
        bool IsEqual(const TouchpadPointer& p) const {
            return (pressed == p.pressed) && (startPosition[0] == p.startPosition[0]) && (startPosition[1] == p.startPosition[1]) && (position[0] == p.position[0]) && (position[1] == p.position[1]);
        }
};


} /* namespace: udptouchpad */

