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

    void EmscriptenMainLoop()
    {
        Application::Render();
    }

    void Application::Run(const DeviceContext &context)
    {
        if (!glfwInit())
        {
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *window = glfwCreateWindow(640, 480, "WebGPU window", nullptr, nullptr);

        if (!m_renderer.Initialize(context))
        {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return;
        }

#if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(EmscriptenMainLoop, 0, false);
#else
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            m_renderer.Render();
        }
#endif
    }
}