#pragma once

#include "pong/Device.h"
#include "pong/Model.h"

#include <webgpu/webgpu_cpp.h>

#include <memory>

namespace pong
{
    class Renderer
    {
    private:
        struct Uniforms
        {
            float time;
        };

        const wgpu::TextureFormat c_swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;

        wgpu::Instance m_instance = {};
        wgpu::Device m_device = {};
        wgpu::Surface m_surface = {};
        wgpu::Queue m_queue = {};

        wgpu::RenderPipeline m_pipeline = {};
        wgpu::SwapChain m_swapChain = {};
        wgpu::ShaderModule m_shaderModule = {};
        wgpu::BindGroupLayout m_bindGroupLayout = {};
        wgpu::BindGroup m_bindGroup = {};

        wgpu::Buffer m_uniformBuffer = {};

        // Temporary
        std::unique_ptr<Model> m_model;

        bool InitializeSurface();
        bool InitializeSwapChain();
        bool InitializePipeline();
        bool InitializeGeometry();
        bool InitializeUniforms();
        bool InitializeBindGroup();

    public:
        Renderer() {}
        ~Renderer() {}

        void Run(void (*mainLoopCallback)(void));

        bool Initialize(const DeviceContext &context);
        void Render();
        void Tick();
        void Terminate();
    };
}