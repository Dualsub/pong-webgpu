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
        int width = 0;
        int height = 0;

        emscripten_get_canvas_element_size("#canvas", &width, &height);

        if (!m_renderer.Initialize(context, uint32_t(width), uint32_t(height)))
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

        emscripten_set_resize_callback(
            EMSCRIPTEN_EVENT_TARGET_WINDOW,
            this,
            true,
            [](int eventType, const EmscriptenUiEvent *uiEvent, void *userData) -> EM_BOOL
            {
                auto *app = reinterpret_cast<Application *>(userData);
                app->m_renderer.Resize(uiEvent->windowInnerWidth, uiEvent->windowInnerHeight);
                return EM_TRUE;
            });

        emscripten_set_main_loop_arg(
            [](void *arg)
            {
                auto *app = reinterpret_cast<Application *>(arg);
                app->Update(float(1.0f / c_fps));
                app->Render();
            },
            this, c_fps, true);
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