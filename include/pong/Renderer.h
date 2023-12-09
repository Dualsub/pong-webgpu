#pragma once
#include "pong/Device.h"

#include <webgpu/webgpu_cpp.h>

namespace pong
{
    class Renderer
    {
    private:
        const wgpu::TextureFormat c_swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

        wgpu::Instance m_instance = {};
        wgpu::Adapter m_adapter = {};
        wgpu::Surface m_surface = {};
        wgpu::Device m_device = {};
        wgpu::RenderPipeline m_pipeline = {};
        wgpu::SwapChain m_swapChain = {};
        wgpu::ShaderModule m_shaderModule = {};

        bool InitializeSurface();
        bool InitializeSwapChain();
        bool InitializePipeline();

    public:
        Renderer() {}
        ~Renderer() {}

        void Run(void (*mainLoopCallback)(void));

        bool Initialize(const DeviceContext &context);
        void Render();
        void Terminate();
    };
}