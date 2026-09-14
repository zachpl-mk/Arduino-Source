/*  Nintendo Switch Joycon
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Containers/Pimpl.tpp"
#include "ControllerInput/ControllerInput.h"
#include "ControllerInput/Gamepad/GamepadInput_State.h"
#include "ControllerInput/Keyboard/KeyboardInput_State.h"
#include "Controllers/ControllerTypes.h"
#include "Controllers/RumbleListener.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Controllers/NintendoSwitch_VirtualControllerState.h"
#include "NintendoSwitch_JoyconState.h"
#include "NintendoSwitch_Joycon.h"

//#include <iostream>
//using std::cout;
//using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{

using namespace std::chrono_literals;


const char JoyconController::NAME[] = "Nintendo Switch: Joycon";
const char LeftJoycon::NAME[] = "Nintendo Switch: Left Joycon";
const char RightJoycon::NAME[] = "Nintendo Switch: Right Joycon";



struct JoyconController::Data{
    ListenerSet<RumbleListener> m_rumble_listeners;
    std::map<KeyboardKey, JoyconDeltas> m_keyboard_mapping;
    JoyconState m_keyboard_state;
    JoyconState m_gamepad_state;
};


namespace{

JoyconState gamepad_joycon_state(const GamepadInputState& state, ControllerClass controller_class){
    JoyconState output;
    if (controller_class == ControllerClass::NintendoSwitch_LeftJoycon){
        if (state.pressed(GAMEPAD_DPAD_UP)) output.buttons |= BUTTON_UP;
        if (state.pressed(GAMEPAD_DPAD_RIGHT)) output.buttons |= BUTTON_RIGHT;
        if (state.pressed(GAMEPAD_DPAD_DOWN)) output.buttons |= BUTTON_DOWN;
        if (state.pressed(GAMEPAD_DPAD_LEFT)) output.buttons |= BUTTON_LEFT;
        if (state.pressed(GAMEPAD_LB)) output.buttons |= BUTTON_L;
        if (state.left_trigger > 0.25) output.buttons |= BUTTON_ZL;
        if (state.pressed(GAMEPAD_LCLICK)) output.buttons |= BUTTON_LCLICK;
        if (state.pressed(GAMEPAD_BACK)) output.buttons |= BUTTON_MINUS;
        output.joystick = JoystickPosition(state.left_x, state.left_y);
    }else{
        if (state.pressed(GAMEPAD_A)) output.buttons |= BUTTON_A;
        if (state.pressed(GAMEPAD_B)) output.buttons |= BUTTON_B;
        if (state.pressed(GAMEPAD_X)) output.buttons |= BUTTON_X;
        if (state.pressed(GAMEPAD_Y)) output.buttons |= BUTTON_Y;
        if (state.pressed(GAMEPAD_RB)) output.buttons |= BUTTON_R;
        if (state.right_trigger > 0.25) output.buttons |= BUTTON_ZR;
        if (state.pressed(GAMEPAD_RCLICK)) output.buttons |= BUTTON_RCLICK;
        if (state.pressed(GAMEPAD_START)) output.buttons |= BUTTON_PLUS;
        if (state.pressed(GAMEPAD_GUIDE)) output.buttons |= BUTTON_HOME;
        output.joystick = JoystickPosition(state.right_x, state.right_y);
    }
    return output;
}

JoyconState merged_joycon_state(const JoyconState& keyboard, const JoyconState& gamepad){
    JoyconState state;
    state.buttons = keyboard.buttons | gamepad.buttons;
    state.joystick = gamepad.joystick.is_neutral() ? keyboard.joystick : gamepad.joystick;
    return state;
}

}



void JoyconController::add_listener(RumbleListener& listener){
    m_data->m_rumble_listeners.add(listener);
}
void JoyconController::remove_listener(RumbleListener& listener){
    m_data->m_rumble_listeners.remove(listener);
}




JoyconController::JoyconController(Logger& logger, ControllerClass controller_class)
    : m_data(CONSTRUCT_TOKEN)
{
    std::vector<std::shared_ptr<EditableTableRow>> mapping =
        controller_class == ControllerClass::NintendoSwitch_LeftJoycon
            ? ConsoleSettings::instance().KEYBOARD_MAPPINGS.LEFT_JOYCON2.current_refs()
            : ConsoleSettings::instance().KEYBOARD_MAPPINGS.RIGHT_JOYCON2.current_refs();

    for (const auto& deltas : mapping){
        const JoyconFromKeyboardTableRow& row = static_cast<const JoyconFromKeyboardTableRow&>(*deltas);
        m_data->m_keyboard_mapping[row.key] += row.snapshot();
    }
}
JoyconController::~JoyconController(){
}




void JoyconController::run_controller_input(const ControllerInputState& state){
    if (state.type() == ControllerInputType::HID_Keyboard){
        JoyconDeltas deltas;
        const KeyboardInputState& lstate = static_cast<const KeyboardInputState&>(state);
        const std::map<KeyboardKey, JoyconDeltas>& map = m_data->m_keyboard_mapping;
        for (KeyboardKey key : lstate.keys()){
            auto iter = map.find(key);
            if (iter != map.end()){
                deltas += iter->second;
            }
        }
        deltas.to_state(m_data->m_keyboard_state);
    }else if (state.type() == ControllerInputType::StandardGamepad){
        const GamepadInputState& gamepad = static_cast<const GamepadInputState&>(state);
        m_data->m_gamepad_state = gamepad_joycon_state(gamepad, controller_class());
    }else{
        return;
    }

    JoyconState controller_state = merged_joycon_state(m_data->m_keyboard_state, m_data->m_gamepad_state);

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


void JoyconController::on_rumble(double magnitude){
    m_data->m_rumble_listeners.run_method(&RumbleListener::on_rumble, magnitude);
}






ControllerClass LeftJoycon::controller_class() const{
    return ControllerClass::NintendoSwitch_LeftJoycon;
}
ControllerClass RightJoycon::controller_class() const{
    return ControllerClass::NintendoSwitch_RightJoycon;
}





}
}
