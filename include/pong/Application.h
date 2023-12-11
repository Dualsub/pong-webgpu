#pragma once
#include "pong/Renderer.h"
#include "pong/Game.h"

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
        Game m_game;

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

        void Initialize();
        void Update(float deltaTime);
        void Render();
        void Terminate();

        void Run(const DeviceContext &context);
    };
}