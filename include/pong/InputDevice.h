#pragma once

namespace pong
{
    class InputDevice
    {
    public:
        InputDevice() = default;
        ~InputDevice() = default;

        bool Initialize();
    };
}