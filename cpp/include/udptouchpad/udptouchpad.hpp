/**
 * @file udptouchpad.hpp
 * @author Robert Damerius (damerius.mail@gmail.com)
 * @brief This is the main header file for the UDPTouchpad library.
 * @date 2024-07-21
 * 
 * @copyright Copyright (c) 2024
 */
#pragma once


/* main API */
#include <udptouchpad/detail/EventSystem.hpp>
#include <udptouchpad/detail/Events.hpp>
#include <udptouchpad/detail/TouchpadPointer.hpp>


/* implementation details */
#include <udptouchpad/detail/Common.hpp>
#include <udptouchpad/detail/MulticastSocket.hpp>
#include <udptouchpad/detail/NonCopyable.hpp>
#include <udptouchpad/detail/ConditionVariable.hpp>
#include <udptouchpad/detail/EventSystemNetworkBase.hpp>
#include <udptouchpad/detail/TouchpadMessage.hpp>
#include <udptouchpad/detail/CircularFIFOBuffer.hpp>
#include <udptouchpad/detail/DeviceDatabase.hpp>
#include <udptouchpad/detail/DeviceData.hpp>

