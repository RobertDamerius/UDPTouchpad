#pragma once


#include <udptouchpad/detail/Common.hpp>


namespace udptouchpad {


namespace detail {


/**
 * @brief Represents a UDP socket to be used to receive multicast traffic from the UDP touchpad app.
 */
class MulticastSocket {
    public:
        /**
         * @brief Construct a new multicast socket object.
         */
        MulticastSocket(): _socket(-1) {
            // on windows, WSAStartup has to be called to allow the use of network sockets
            // multiple calls are possible and we never call WSACleanup
            #ifdef _WIN32
            WSADATA wsadata;
            (void) WSAStartup(MAKEWORD(2, 2), &wsadata);
            #endif
        }

        /**
         * @brief Destroy the multicast socket object.
         */
        ~MulticastSocket(){
            Close();
        }

        /**
         * @brief Open the multicast socket to receive messages from the UDP touchpad app.
         * @return True if success or already open, false otherwise.
         * @details If this member function fails, use @ref GetErrorString to obtain error information.
         */
        bool Open(void){
            if(-1 == _socket){
                if(!OpenSocket()){
                    CloseSocket();
                    return false;
                }
                ReusePort();
                if(!BindPort(UDP_TOUCHPAD_MULTICAST_DESTINATION_PORT)){
                    CloseSocket();
                    return false;
                }
                interfaceNames = GetAllInterfaceNames();
                if(!JoinMulticastGroupOnAllInterfaces(UDP_TOUCHPAD_MULTICAST_GROUP_ADDRESS, interfaceNames)){
                    LeaveMulticastGroupOnAllInterfaces(UDP_TOUCHPAD_MULTICAST_GROUP_ADDRESS, interfaceNames);
                    CloseSocket();
                    return false;
                }
                ResetLastError();
            }
            return true;
        }

        /**
         * @brief Close the multicast socket.
         */
        void Close(void){
            LeaveMulticastGroupOnAllInterfaces(UDP_TOUCHPAD_MULTICAST_GROUP_ADDRESS, interfaceNames);
            CloseSocket();
        }

        /**
         * @brief Check whether the socket is open or not.
         * @return True if open, false otherwise.
         */
        bool IsOpen(void){ return -1 != _socket; }

        /**
         * @brief Get bytes from the receive buffer of the operating system.
         * @param[out] sourceIP Source, where to store the sender IPv4 address, that sent the message.
         * @param[out] bytes Pointer to data array, where received bytes should be stored.
         * @param[in] maxSize The maximum size of the data array.
         * @return A tuple containing the number of bytes that have been received and an OS-specific error code.
         */
        std::tuple<int32_t, int32_t> ReceiveFrom(uint32_t& sourceIP, uint8_t *bytes, int32_t maxSize){
            sockaddr_in addr{};

            #ifdef _WIN32
            int address_size = sizeof(addr);
            #elif __linux__
            socklen_t address_size = sizeof(addr);
            #else
            #error "Platform is not supported!"
            #endif

            #ifdef _WIN32
            WSASetLastError(0);
            #elif __linux__
            errno = 0;
            #else
            #error "Platform is not supported!"
            #endif

            int rx = recvfrom(_socket, reinterpret_cast<char*>(bytes), maxSize, 0, reinterpret_cast<struct sockaddr*>(&addr), &address_size);

            #ifdef _WIN32
            int errorCode = static_cast<int>(WSAGetLastError());
            #elif __linux__
            int errorCode = static_cast<int>(errno);
            #else
            #error "Platform is not supported!"
            #endif

            #ifdef _WIN32
            sourceIP = ntohl(addr.sin_addr.S_un.S_addr);
            #elif __linux__
            sourceIP = ntohl(addr.sin_addr.s_addr);
            #else
            #error "Platform is not supported!"
            #endif

            return std::make_tuple(static_cast<int32_t>(rx), static_cast<int32_t>(errorCode));
        }

        /**
         * @brief Get the last error string that has been set by @ref Open.
         * @return String giving information about the last error.
         */
        std::string GetErrorString(void){ return errorString; }

    private:
        std::atomic<int32_t> _socket;              // Socket object.
        std::string errorString;                   // OS-specific error string, set if @ref Open fails.
        std::vector<std::string> interfaceNames;   // List of all interface names to be used for joining multicast groups.

        /**
         * @brief Open the actual socket and enable reuse port option.
         * @return True if success, false otherwise
         */
        bool OpenSocket(void){
            ResetLastError();
            _socket = socket(AF_INET, SOCK_DGRAM, 0);
            if(_socket < 0){
                errorString = GenerateErrorString("Failed to open socket!");
                _socket = -1;
                return false;
            }

            // special option to fix windows bug
            #ifdef _WIN32
            BOOL bNewBehavior = FALSE;
            DWORD dwBytesReturned = 0;
            WSAIoctl(_socket, _WSAIOW(IOC_VENDOR, 12), &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
            #endif
            return true;
        }

        /**
         * @brief Close the actual socket.
         */
        void CloseSocket(void){
            if(-1 != _socket){
                #ifdef _WIN32
                (void) shutdown(_socket, SD_BOTH);
                (void) closesocket(_socket);
                #elif __linux__
                (void) shutdown(_socket, SHUT_RDWR);
                (void) close(_socket);
                #else
                #error "Platform is not supported!"
                #endif
            }
            _socket = -1;
        }

        /**
         * @brief Set socket option to reuse the port to allow multiple sockets to use the same port number.
         */
        void ReusePort(void){
            unsigned reuse = 1;

            #ifdef _WIN32
            (void) SetOption(SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(reuse));
            #elif __linux__
            (void) SetOption(SOL_SOCKET, SO_REUSEPORT, (const void*)&reuse, sizeof(reuse));
            #else
            #error "Platform is not supported!"
            #endif
        }

        /**
         * @brief Bind a port to an open socket object.
         * @param[in] port A port that should be bound to the socket.
         * @return True if success, false otherwise.
         */
        bool BindPort(uint16_t port){
            struct sockaddr_in addr_this;
            addr_this.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_this.sin_family = AF_INET;
            addr_this.sin_port = htons(port);
            int s = static_cast<int>(_socket);
            ResetLastError();
            if(bind(s, (struct sockaddr *)&addr_this, sizeof(struct sockaddr_in)) < 0){
                errorString = GenerateErrorString("Failed to bind port!");
                return false;
            }
            return true;
        }

        /**
         * @brief Get all network interface names.
         * @return List of all network interface names. On windows, only the default interface ("0.0.0.0") is returned.
         */
        std::vector<std::string> GetAllInterfaceNames(void){
            std::vector<std::string> result;

            #ifdef _WIN32
            // windows GetAdaptersInfo is buggy as hell, so we leave it empty
            #elif __linux__
            struct if_nameindex *if_ni, *i;
            if_ni = if_nameindex();
            if(if_ni){
                for(i = if_ni; !(i->if_index == 0 && i->if_name == nullptr); i++){
                    result.push_back(std::string(i->if_name));
                }
                if_freenameindex(if_ni);
            }
            #else
            #error "Platform is not supported!"
            #endif

            return result;
        }

        /**
         * @brief Join a multicast group on a given list of network interfaces.
         * @param[in] strGroupAddress The group address to be joined.
         * @param[in] interfaceNames List of network interfaces on which to join.
         * @return True if success, false otherwise.
         */
        bool JoinMulticastGroupOnAllInterfaces(const char* strGroupAddress, std::vector<std::string> interfaceNames){
            if(interfaceNames.empty()){
                #ifdef _WIN32
                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                mreq.imr_interface.s_addr = inet_addr("0.0.0.0");
                #elif __linux__
                struct ip_mreqn mreq;
                mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                mreq.imr_address.s_addr = htonl(INADDR_ANY);
                mreq.imr_ifindex = 0;
                #else
                #error "Platform is not supported!"
                #endif

                ResetLastError();
                if(SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void*) &mreq, sizeof(mreq)) < 0){
                    errorString = GenerateErrorString("Failed to join multicast group!");
                    return false;
                }
            }
            else{
                for(auto&& interfaceName : interfaceNames){
                    #ifdef _WIN32
                    uint8_t index = static_cast<uint8_t>(if_nametoindex(interfaceName.c_str()));
                    char strInterface[16];
                    sprintf(&strInterface[0], "0.0.0.%u", index);
                    struct ip_mreq mreq;
                    mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                    mreq.imr_interface.s_addr = inet_addr(strInterface);
                    #elif __linux__
                    struct ip_mreqn mreq;
                    mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                    mreq.imr_address.s_addr = htonl(INADDR_ANY);
                    mreq.imr_ifindex = if_nametoindex(interfaceName.c_str());
                    #else
                    #error "Platform is not supported!"
                    #endif

                    ResetLastError();
                    if(SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void*) &mreq, sizeof(mreq)) < 0){
                        errorString = GenerateErrorString("Failed to join multicast group on interface \"" + interfaceName + "\"!");
                        return false;
                    }
                }
            }
            return true;
        }

        /**
         * @brief Leave a multicast group on a given list of network interfaces.
         * @param[in] strGroupAddress The group address to be left.
         * @param[in] interfaceNames List of network interfaces on which to leave.
         */
        void LeaveMulticastGroupOnAllInterfaces(const char* strGroupAddress, std::vector<std::string> interfaceNames){
            if(interfaceNames.empty()){
                #ifdef _WIN32
                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                mreq.imr_interface.s_addr = inet_addr("0.0.0.0");
                #elif __linux__
                struct ip_mreqn mreq;
                mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                mreq.imr_address.s_addr = htonl(INADDR_ANY);
                mreq.imr_ifindex = 0;
                #else
                #error "Platform is not supported!"
                #endif

                (void) SetOption(IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void*) &mreq, sizeof(mreq));
            }
            else{
                for(auto&& interfaceName : interfaceNames){
                    #ifdef _WIN32
                    uint8_t index = static_cast<uint8_t>(if_nametoindex(interfaceName.c_str()));
                    char strInterface[16];
                    sprintf(&strInterface[0], "0.0.0.%u", index);
                    struct ip_mreq mreq;
                    mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                    mreq.imr_interface.s_addr = inet_addr(strInterface);
                    #elif __linux__
                    struct ip_mreqn mreq;
                    mreq.imr_multiaddr.s_addr = inet_addr(strGroupAddress);
                    mreq.imr_address.s_addr = htonl(INADDR_ANY);
                    mreq.imr_ifindex = if_nametoindex(interfaceName.c_str());
                    #else
                    #error "Platform is not supported!"
                    #endif

                    (void) SetOption(IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void*) &mreq, sizeof(mreq));
                }
            }
        }

        /**
         * @brief Set socket options using the setsockopt() function.
         * @param[in] level The level at which the option is defined (for example, SOL_SOCKET).
         * @param[in] optname The socket option for which the value is to be set (for example, SO_BROADCAST).
         * @param[in] optval A pointer to the buffer in which the value for the requested option is specified.
         * @param[in] optlen The size, in bytes, of the buffer pointed to by the optval parameter.
         * @return If no error occurs, zero is returned.
         */
        int SetOption(int level, int optname, const void *optval, int optlen){
            #ifdef _WIN32
            return setsockopt(_socket, level, optname, reinterpret_cast<const char*>(optval), optlen);
            #elif __linux__
            return setsockopt(_socket, level, optname, optval, static_cast<socklen_t>(optlen));
            #else
            #error "Platform is not supported!"
            #endif
        }

        /**
         * @brief Reset the last error to zero.
         */
        void ResetLastError(void){
            #ifdef _WIN32
            WSASetLastError(0);
            #elif __linux__
            errno = 0;
            #else
            #error "Platform is not supported!"
            #endif

            errorString.clear();
        }

        /**
         * @brief Get the last error value.
         * @return String representing the last error code.
         */
        std::string GenerateErrorString(std::string prestring){
            #ifdef _WIN32
            int err = static_cast<int>(WSAGetLastError());
            std::string errStr("");
            #elif __linux__
            int err = static_cast<int>(errno);
            std::string errStr = std::string(strerror(err)) + std::string(" ");
            #else
            #error "Platform is not supported!"
            #endif

            return prestring + std::string(" ") + errStr + std::string("(") + std::to_string(err) + std::string(")");
        }
};


} /* namespace: detail */


} /* namespace: udptouchpad */

