#include "pong/InputDevice.h"

#include "pong/Application.h"

#include <emscripten/emscripten.h>

namespace pong
{
    bool InputDevice::Initialize()
    {
        emscripten_set_keydown_callback(
            EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true,
            [](int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) -> EM_BOOL
            {
                if (keyEvent->repeat)
                {
                    return false;
                }

                Connection &connection = Application::GetConnection();

                if (keyEvent->keyCode == 37)
                {
                    connection.SendPressedUp(true);
                }
                else if (keyEvent->keyCode == 39)
                {
                    connection.SendPressedDown(true);
                }

                return true;
            });

        emscripten_set_keyup_callback(
            EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true,
            [](int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) -> EM_BOOL
            {
                if (keyEvent->repeat)
                {
                    return false;
                }

                Connection &connection = Application::GetConnection();

                // Up arrow
                if (keyEvent->keyCode == 38)
                {
                    connection.SendPressedUp(false);
                }
                else if (keyEvent->keyCode == 40)
                {
                    connection.SendPressedDown(false);
                }

                return true;
            });

        return true;
    }
}