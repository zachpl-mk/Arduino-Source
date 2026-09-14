/*  Nintendo Switch Controller
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Containers/Pimpl.tpp"
#include "ControllerInput/ControllerInput.h"
#include "ControllerInput/Gamepad/GamepadInput_State.h"
#include "ControllerInput/Keyboard/KeyboardInput_State.h"
#include "Controllers/RumbleListener.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Controllers/NintendoSwitch_VirtualControllerState.h"
#include "NintendoSwitch_ProControllerState.h"
#include "NintendoSwitch_ProController.h"

//#include <iostream>
//using std::cout;
//using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{

using namespace std::chrono_literals;


const char ProController::NAME[] = "Nintendo Switch: Pro Controller";





struct ProController::Data{
    ListenerSet<RumbleListener> m_rumble_listeners;
    std::map<KeyboardKey, ProControllerDeltas> m_keyboard_mapping;
    ProControllerState m_keyboard_state;
    ProControllerState m_gamepad_state;
};


namespace{

Button gamepad_buttons(const GamepadInputState& state){
    Button buttons = BUTTON_NONE;
    if (state.pressed(GAMEPAD_A)) buttons |= BUTTON_A;
    if (state.pressed(GAMEPAD_B)) buttons |= BUTTON_B;
    if (state.pressed(GAMEPAD_X)) buttons |= BUTTON_X;
    if (state.pressed(GAMEPAD_Y)) buttons |= BUTTON_Y;
    if (state.pressed(GAMEPAD_LB)) buttons |= BUTTON_L;
    if (state.pressed(GAMEPAD_RB)) buttons |= BUTTON_R;
    if (state.pressed(GAMEPAD_BACK)) buttons |= BUTTON_MINUS;
    if (state.pressed(GAMEPAD_START)) buttons |= BUTTON_PLUS;
    if (state.pressed(GAMEPAD_LCLICK)) buttons |= BUTTON_LCLICK;
    if (state.pressed(GAMEPAD_RCLICK)) buttons |= BUTTON_RCLICK;
    if (state.pressed(GAMEPAD_GUIDE)) buttons |= BUTTON_HOME;
    if (state.left_trigger > 0.25) buttons |= BUTTON_ZL;
    if (state.right_trigger > 0.25) buttons |= BUTTON_ZR;
    return buttons;
}

DpadPosition gamepad_dpad(const GamepadInputState& state){
    int x = 0;
    int y = 0;
    if (state.pressed(GAMEPAD_DPAD_LEFT)) x--;
    if (state.pressed(GAMEPAD_DPAD_RIGHT)) x++;
    if (state.pressed(GAMEPAD_DPAD_UP)) y++;
    if (state.pressed(GAMEPAD_DPAD_DOWN)) y--;

    if (x == 0 && y > 0) return DPAD_UP;
    if (x > 0 && y > 0) return DPAD_UP_RIGHT;
    if (x > 0 && y == 0) return DPAD_RIGHT;
    if (x > 0 && y < 0) return DPAD_DOWN_RIGHT;
    if (x == 0 && y < 0) return DPAD_DOWN;
    if (x < 0 && y < 0) return DPAD_DOWN_LEFT;
    if (x < 0 && y == 0) return DPAD_LEFT;
    if (x < 0 && y > 0) return DPAD_UP_LEFT;
    return DPAD_NONE;
}

JoystickPosition merged_stick(const JoystickPosition& keyboard, const JoystickPosition& gamepad){
    return gamepad.is_neutral() ? keyboard : gamepad;
}

ProControllerState merged_state(const ProControllerState& keyboard, const ProControllerState& gamepad){
    ProControllerState state;
    state.buttons = keyboard.buttons | gamepad.buttons;
    state.dpad = gamepad.dpad == DPAD_NONE ? keyboard.dpad : gamepad.dpad;
    state.left_joystick = merged_stick(keyboard.left_joystick, gamepad.left_joystick);
    state.right_joystick = merged_stick(keyboard.right_joystick, gamepad.right_joystick);
    return state;
}

}



void ProController::add_listener(RumbleListener& listener){
    m_data->m_rumble_listeners.add(listener);
}
void ProController::remove_listener(RumbleListener& listener){
    m_data->m_rumble_listeners.remove(listener);
}




ProController::ProController(Logger& logger)
    : m_data(CONSTRUCT_TOKEN)
{
    std::vector<std::shared_ptr<EditableTableRow>> mapping =
        ConsoleSettings::instance().KEYBOARD_MAPPINGS.PRO_CONTROLLER2.current_refs();

    for (const auto& deltas : mapping){
        const ProControllerFromKeyboardTableRow& row = static_cast<const ProControllerFromKeyboardTableRow&>(*deltas);
        m_data->m_keyboard_mapping[row.key] += row.snapshot();
    }
}
ProController::~ProController(){
}

ControllerClass ProController::controller_class() const{
    return ControllerClass::NintendoSwitch_ProController;
}




void ProController::run_controller_input(const ControllerInputState& state){
//    cout << "run_controller_input()" << endl;

    if (state.type() == ControllerInputType::HID_Keyboard){
        ProControllerDeltas deltas;
        const KeyboardInputState& lstate = static_cast<const KeyboardInputState&>(state);
        const std::map<KeyboardKey, ProControllerDeltas>& map = m_data->m_keyboard_mapping;
        for (KeyboardKey key : lstate.keys()){
            auto iter = map.find(key);
            if (iter != map.end()){
                deltas += iter->second;
            }
        }
        deltas.to_state(m_data->m_keyboard_state);
    }else if (state.type() == ControllerInputType::StandardGamepad){
        const GamepadInputState& gamepad = static_cast<const GamepadInputState&>(state);
        ProControllerState& output = m_data->m_gamepad_state;
        output.buttons = gamepad_buttons(gamepad);
        output.dpad = gamepad_dpad(gamepad);
        output.left_joystick = JoystickPosition(gamepad.left_x, gamepad.left_y);
        output.right_joystick = JoystickPosition(gamepad.right_x, gamepad.right_y);
    }else{
        return;
    }

    ProControllerState controller_state = merged_state(m_data->m_keyboard_state, m_data->m_gamepad_state);

    WallClock timestamp;
    if (controller_state.is_neutral()){
        timestamp = current_time();
        cancel_all_commands();
    }else{
        replace_on_next_command();

        timestamp = current_time();
        controller_state.execute(nullptr, false, *this, 2000ms);
    }

    on_command_input(timestamp, controller_state);
}


void ProController::on_rumble(double magnitude){
    m_data->m_rumble_listeners.run_method(&RumbleListener::on_rumble, magnitude);
}





}
}
