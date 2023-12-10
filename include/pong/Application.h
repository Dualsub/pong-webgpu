#pragma once
#include "pong/Renderer.h"

#include <iostream>
#include <cassert>

namespace pong
{
    class Application
    {
    private:
        static const uint32_t c_fps = 60;
        static Application *s_instance;
        Renderer m_renderer;

    public:
        Application()
        {
            s_instance = this;
            std::cout << "Application created" << std::endl;
        }
        ~Application()
        {
            s_instance = nullptr;
            std::cout << "Application destroyed" << std::endl;
        }

        static Application *GetInstance() { return s_instance; }

        // For Emscripten
        static void Render()
        {
            assert(s_instance);
            s_instance->m_renderer.Render();
        }

        void Run(const DeviceContext &context);
    };
}