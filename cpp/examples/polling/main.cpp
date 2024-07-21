#include <udptouchpad.hpp>
#include <iostream>


static void CallbackError(udptouchpad::ErrorEvent e){
    std::cerr << "[ERROR] " << e.ToString() << "\n";
}

static void CallbackTouchpad(udptouchpad::TouchpadEvent e){
    std::cerr << "[TOUCHPAD] " << e.ToString() << "\n";
}


int main(int, char**){
    // create an event system (event buffer size: 16 error events, 256 touchpad events)
    udptouchpad::EventSystem<16,256> eventSystem;

    // set user-defined callback functions
    eventSystem.SetErrorCallback(CallbackError);
    eventSystem.SetTouchpadCallback(CallbackTouchpad);

    // poll events for about 5 seconds
    for(int i = 0; i < 50; ++i){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        eventSystem.PollEvents();
    }
    return 0;
}
