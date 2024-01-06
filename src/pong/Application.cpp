#include "pong/Application.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <webgpu/webgpu_glfw.h>
#endif

#include <iostream>

namespace pong
{
    Application *Application::s_instance = nullptr;

    void Application::Run(const DeviceContext &context)
    {
        // if (!glfwInit())
        // {
        //     return;
        // }

        // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // GLFWwindow *window = glfwCreateWindow(640, 480, "WebGPU window", nullptr, nullptr);

        if (!m_renderer.Initialize(context))
        {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return;
        }

        if (!m_inputDevice.Initialize())
        {
            std::cerr << "Failed to initialize input device" << std::endl;
            return;
        }

        if (!m_audioPlayer.Initialize())
        {
            std::cerr << "Failed to initialize audio player" << std::endl;
            return;
        }

        Initialize();

#if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop_arg(
            [](void *arg)
            {
                auto *app = reinterpret_cast<Application *>(arg);
                app->Update(float(1.0f / c_fps));
                app->Render();
            },
            this, c_fps, true);
#else
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            m_renderer.Render();
        }
#endif
    }

    void Application::Initialize()
    {
        m_game.Initialize(m_renderer);
    }

    void Application::Update(float deltaTime)
    {
        m_game.Update(deltaTime);
    }

    void Application::Render()
    {
        m_game.Render(m_renderer);
        m_renderer.Render();
    }

    void Application::Terminate()
    {
        m_audioPlayer.Terminate();
        m_renderer.Terminate();
    }

}