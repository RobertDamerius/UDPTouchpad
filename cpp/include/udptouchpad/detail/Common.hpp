#pragma once


/* Default C/C++ headers */
#include <cstdint>
#include <string>
#include <atomic>
#include <tuple>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <bit>
#include <functional>
#include <unordered_map>


/* OS depending */
// Windows System (MinGW)
#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
// Unix System
#elif __linux__
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#else
#error "Platform is not supported!"
#endif


#define UDP_TOUCHPAD_MULTICAST_GROUP_ADDRESS "239.192.82.74"
#define UDP_TOUCHPAD_MULTICAST_DESTINATION_PORT (10891)
#define UDP_TOUCHPAD_REOPEN_SOCKET_RETRY_TIME_MS (1000)

