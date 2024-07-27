#include <udptouchpad.hpp>
#include <iostream>
#include <csignal>


static void CallbackError(udptouchpad::ErrorEvent e){
    std::cerr << "[ERROR] " << e.ToString() << "\n";
}

static void CallbackDeviceConnection(udptouchpad::DeviceConnectionEvent e){
    std::cerr << "[DEVCON] " << e.ToString() << "\n";
}

static void CallbackTouchpadPointer(udptouchpad::TouchpadPointerEvent e){
    std::cerr << "[POINTER] " << e.ToString() << "\n";
}

static void CallbackMotionSensor(udptouchpad::MotionSensorEvent e){
    std::cerr << "[MOTION] " << e.ToString() << "\n";
}

static bool terminate = false;
static void SignalHandler(int){ terminate = true; }


int main(int, char**){
    std::signal(SIGINT, &SignalHandler);
    std::signal(SIGTERM, &SignalHandler);
    std::cerr << "Running example\nPress Ctrl+C to terminate\n";

    // create an event system
    udptouchpad::EventSystem eventSystem;

    // set user-defined callback functions
    eventSystem.SetErrorCallback(CallbackError);
    eventSystem.SetDeviceConnectionCallback(CallbackDeviceConnection);
    eventSystem.SetTouchpadPointerCallback(CallbackTouchpadPointer);
    eventSystem.SetMotionSensorCallback(CallbackMotionSensor);

    // start with a fresh and clean event system and poll events
    eventSystem.Clear();
    while(!terminate){
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        eventSystem.PollEvents();
    }
    return 0;
}

