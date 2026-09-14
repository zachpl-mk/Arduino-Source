/*  Global Gamepad Tracker
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include "GlobalGamepadTracker.h"

#ifdef _WIN32
#include <Windows.h>
#include <Xinput.h>
#endif

namespace PokemonAutomation{

using namespace std::chrono_literals;


GamepadTracker& global_gamepad_tracker(){
    static GamepadTracker tracker;
    return tracker;
}


#ifdef _WIN32
namespace{

using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);

XInputGetStateFn load_xinput(){
    static HMODULE module = []{
        const wchar_t* libraries[] = {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"};
        for (const wchar_t* library : libraries){
            if (HMODULE handle = LoadLibraryW(library)){
                return handle;
            }
        }
        return (HMODULE)nullptr;
    }();
    if (module == nullptr){
        return nullptr;
    }
    return reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
}

double normalize_stick(SHORT value, SHORT deadzone){
    const int magnitude = std::abs((int)value);
    if (magnitude <= deadzone){
        return 0;
    }
    const double sign = value < 0 ? -1.0 : 1.0;
    const double range = value < 0 ? 32768.0 : 32767.0;
    return sign * std::clamp((magnitude - deadzone) / (range - deadzone), 0.0, 1.0);
}

double normalize_trigger(BYTE value){
    if (value <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD){
        return 0;
    }
    return (value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (255.0 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

GamepadInputState read_xinput(const XINPUT_GAMEPAD& pad){
    GamepadInputState state;
    auto add = [&](WORD mask, GamepadButton button){
        if ((pad.wButtons & mask) != 0){
            state.buttons |= button;
        }
    };

    add(XINPUT_GAMEPAD_A, GAMEPAD_A);
    add(XINPUT_GAMEPAD_B, GAMEPAD_B);
    add(XINPUT_GAMEPAD_X, GAMEPAD_X);
    add(XINPUT_GAMEPAD_Y, GAMEPAD_Y);
    add(XINPUT_GAMEPAD_LEFT_SHOULDER, GAMEPAD_LB);
    add(XINPUT_GAMEPAD_RIGHT_SHOULDER, GAMEPAD_RB);
    add(XINPUT_GAMEPAD_BACK, GAMEPAD_BACK);
    add(XINPUT_GAMEPAD_START, GAMEPAD_START);
    add(XINPUT_GAMEPAD_LEFT_THUMB, GAMEPAD_LCLICK);
    add(XINPUT_GAMEPAD_RIGHT_THUMB, GAMEPAD_RCLICK);
    add(XINPUT_GAMEPAD_DPAD_UP, GAMEPAD_DPAD_UP);
    add(XINPUT_GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_DOWN);
    add(XINPUT_GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_LEFT);
    add(XINPUT_GAMEPAD_DPAD_RIGHT, GAMEPAD_DPAD_RIGHT);

    state.left_trigger = normalize_trigger(pad.bLeftTrigger);
    state.right_trigger = normalize_trigger(pad.bRightTrigger);
    state.left_x = normalize_stick(pad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    state.left_y = normalize_stick(pad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    state.right_x = normalize_stick(pad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    state.right_y = normalize_stick(pad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    return state;
}

}
#endif


GamepadTracker::GamepadTracker()
    : m_thread([this]{ thread_loop(); })
{}

GamepadTracker::~GamepadTracker(){
    stop();
}

void GamepadTracker::stop() noexcept{
    if (!m_thread.joinable()){
        return;
    }
    m_stopping.store(true, std::memory_order_release);
    m_thread.join();
}

void GamepadTracker::clear_state(){
    m_clear_requested.store(true, std::memory_order_release);
}

void GamepadTracker::thread_loop(){
    GamepadInputState last;
    bool have_last = false;

#ifdef _WIN32
    XInputGetStateFn get_state = load_xinput();
#endif

    while (!m_stopping.load(std::memory_order_acquire)){
        GamepadInputState current;
        bool connected = false;

#ifdef _WIN32
        if (get_state != nullptr){
            for (DWORD index = 0; index < XUSER_MAX_COUNT; index++){
                XINPUT_STATE state{};
                if (get_state(index, &state) == ERROR_SUCCESS){
                    current = read_xinput(state.Gamepad);
                    connected = true;
                    break;
                }
            }
        }
#endif

        if (m_clear_requested.exchange(false, std::memory_order_acq_rel)){
            current.clear();
            connected = false;
        }

        if (connected){
            if (!have_last || current != last){
                m_listeners.run_method(&ControllerInputListener::run_controller_input, current);
                last = current;
                have_last = true;
            }
        }else if (have_last){
            current.clear();
            m_listeners.run_method(&ControllerInputListener::run_controller_input, current);
            last.clear();
            have_last = false;
        }

        std::this_thread::sleep_for(8ms);
    }
}


}
