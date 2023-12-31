#pragma once

#include "pong/AudioPlayer.h"
#include "pong/Connection.h"
#include "pong/InputDevice.h"
#include "pong/Game.h"
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
        Game m_game;
        Connection m_connection;
        InputDevice m_inputDevice;
        AudioPlayer m_audioPlayer;

    public:
        Application()
        {
            s_instance = this;
            std::cout << "Application created" << std::endl;
        }
        ~Application()
        {
            s_instance->Terminate();
            s_instance = nullptr;
            std::cout << "Application destroyed" << std::endl;
        }

        static Application *GetInstance() { return s_instance; }
        static Renderer &GetRenderer() { return s_instance->m_renderer; }
        static Game &GetGame() { return s_instance->m_game; }
        static Connection &GetConnection() { return s_instance->m_connection; }
        static InputDevice &GetInputDevice() { return s_instance->m_inputDevice; }
        static AudioPlayer &GetAudioPlayer() { return s_instance->m_audioPlayer; }

        void Initialize();
        void Update(float deltaTime);
        void Render();
        void Terminate();

        void Run(const DeviceContext &context);
    };
}