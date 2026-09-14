/*  Global Gamepad Tracker
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_ControllerInput_Gamepad_GlobalGamepadTracker_H
#define PokemonAutomation_ControllerInput_Gamepad_GlobalGamepadTracker_H

#include <atomic>
#include <thread>
#include "ControllerInput/ControllerInput.h"
#include "GamepadInput_State.h"

namespace PokemonAutomation{


class GamepadTracker final : public ControllerInputSource{
public:
    GamepadTracker();
    ~GamepadTracker();

    virtual void stop() noexcept override;
    virtual void clear_state() override;

private:
    void thread_loop();

private:
    std::atomic<bool> m_stopping{false};
    std::atomic<bool> m_clear_requested{false};
    std::thread m_thread;
};


GamepadTracker& global_gamepad_tracker();


}
#endif
