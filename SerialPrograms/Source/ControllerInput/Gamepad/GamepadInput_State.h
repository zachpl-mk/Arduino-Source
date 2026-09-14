/*  Gamepad Input State
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_ControllerInput_Gamepad_GamepadInput_State_H
#define PokemonAutomation_ControllerInput_Gamepad_GamepadInput_State_H

#include <cstdint>
#include "ControllerInput/ControllerInput.h"

namespace PokemonAutomation{


enum GamepadButton : uint32_t{
    GAMEPAD_NONE        = 0,
    GAMEPAD_A           = 1u << 0,
    GAMEPAD_B           = 1u << 1,
    GAMEPAD_X           = 1u << 2,
    GAMEPAD_Y           = 1u << 3,
    GAMEPAD_LB          = 1u << 4,
    GAMEPAD_RB          = 1u << 5,
    GAMEPAD_BACK        = 1u << 6,
    GAMEPAD_START       = 1u << 7,
    GAMEPAD_LCLICK      = 1u << 8,
    GAMEPAD_RCLICK      = 1u << 9,
    GAMEPAD_GUIDE       = 1u << 10,
    GAMEPAD_DPAD_UP     = 1u << 11,
    GAMEPAD_DPAD_DOWN   = 1u << 12,
    GAMEPAD_DPAD_LEFT   = 1u << 13,
    GAMEPAD_DPAD_RIGHT  = 1u << 14,
};


class GamepadInputState final : public ControllerInputState{
public:
    GamepadInputState(const GamepadInputState& x)
        : ControllerInputState(ControllerInputType::StandardGamepad)
    {
        *this = x;
    }
    GamepadInputState& operator=(const GamepadInputState& x){
        if (this != &x){
            buttons = x.buttons;
            left_trigger = x.left_trigger;
            right_trigger = x.right_trigger;
            left_x = x.left_x;
            left_y = x.left_y;
            right_x = x.right_x;
            right_y = x.right_y;
        }
        return *this;
    }

    GamepadInputState()
        : ControllerInputState(ControllerInputType::StandardGamepad)
    {}

    virtual void clear() override{
        buttons = GAMEPAD_NONE;
        left_trigger = 0;
        right_trigger = 0;
        left_x = 0;
        left_y = 0;
        right_x = 0;
        right_y = 0;
    }
    virtual bool is_neutral() const override{
        return buttons == GAMEPAD_NONE
            && left_trigger == 0 && right_trigger == 0
            && left_x == 0 && left_y == 0
            && right_x == 0 && right_y == 0;
    }
    virtual bool operator==(const ControllerInputState& state) const override{
        const GamepadInputState* x = dynamic_cast<const GamepadInputState*>(&state);
        return x != nullptr
            && buttons == x->buttons
            && left_trigger == x->left_trigger
            && right_trigger == x->right_trigger
            && left_x == x->left_x && left_y == x->left_y
            && right_x == x->right_x && right_y == x->right_y;
    }

    bool pressed(GamepadButton button) const{
        return (buttons & button) != 0;
    }

public:
    uint32_t buttons = GAMEPAD_NONE;
    double left_trigger = 0;
    double right_trigger = 0;
    double left_x = 0;
    double left_y = 0;
    double right_x = 0;
    double right_y = 0;
};


}
#endif
