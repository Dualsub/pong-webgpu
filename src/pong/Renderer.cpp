#include "pong/Renderer.h"

#include <webgpu/webgpu_cpp.h>

#include <iostream>
#include <cassert>

namespace pong
{
    const char *shaderSource = R"(
    @vertex
    fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
        var p = vec2f(0.0, 0.0);
        if (in_vertex_index == 0u) {
            p = vec2f(-0.5, -0.5);
        } else if (in_vertex_index == 1u) {
            p = vec2f(0.5, -0.5);
        } else {
            p = vec2f(0.0, 0.5);
        }
        return vec4f(p, 0.0, 1.0);
    }

    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4f(0.0, 0.4, 1.0, 1.0);
    }
    )";

    bool Renderer::Initialize(const DeviceContext &context)
    {
        m_instance = context.instance;
        m_device = context.device;

        if (!InitializeSurface())
        {
            std::cerr << "Cannot initialize WebGPU surface" << std::endl;
            return false;
        }

        if (!InitializeSwapChain())
        {
            std::cerr << "Cannot initialize WebGPU swap chain" << std::endl;
            return false;
        }

        if (!InitializePipeline())
        {
            std::cerr << "Cannot initialize WebGPU pipeline" << std::endl;
            return false;
        }

        return true;
    }

    bool Renderer::InitializePipeline()
    {
        std::cout << "Initializing WebGPU pipeline" << std::endl;
        wgpu::RenderPipelineDescriptor pipelineDesc = {};

        std::cout << "  Initializing WebGPU shader module" << std::endl;
        // Shader source.
        wgpu::ShaderModuleDescriptor shaderDesc;
        shaderDesc.label = "Shader Module Descriptor";

        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        // Set the chained struct's header
        shaderCodeDesc.nextInChain = nullptr;
        shaderCodeDesc.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        // Connect the chain
        shaderDesc.nextInChain = &shaderCodeDesc;

        shaderCodeDesc.code = shaderSource;

        auto shaderModule = m_device.CreateShaderModule(&shaderDesc);

        std::cout << "  Initializing WebGPU vertex state" << std::endl;
        // Vertex state.
        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;

        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;

        std::cout << "  Initializing WebGPU primitive state" << std::endl;
        // Primitive state.
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
        pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

        std::cout << "  Initializing WebGPU depth stencil state" << std::endl;
        // Fragment and blend state.
        wgpu::FragmentState fragmentState;
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.constants = nullptr;

        wgpu::BlendState blendState;
        blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.color.operation = wgpu::BlendOperation::Add;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = wgpu::TextureFormat::BGRA8Unorm;
        colorTarget.blend = &blendState;
        colorTarget.writeMask = wgpu::ColorWriteMask::All;

        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        pipelineDesc.fragment = &fragmentState;

        // Disable depth/stencil testing.
        pipelineDesc.depthStencil = nullptr;

        std::cout << "  Initializing WebGPU multisample state" << std::endl;
        // Multisampling is disabled.
        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = ~0u;
        pipelineDesc.multisample.alphaToCoverageEnabled = false;

        // No pipeline layout.
        pipelineDesc.layout = nullptr;

        // Create the pipeline.
        m_pipeline = m_device.CreateRenderPipeline(&pipelineDesc);
        std::cout << "  Initializing WebGPU pipeline" << std::endl;

        return true;
    }

    bool Renderer::InitializeSurface()
    {
        std::cout << "Initializing WebGPU surface" << std::endl;
        wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
        canvasDesc.selector = "#canvas";

        wgpu::SurfaceDescriptor surfaceDesc;
        surfaceDesc.nextInChain = &canvasDesc;
        m_surface = m_instance.CreateSurface(&surfaceDesc);

        return true;
    }

    bool Renderer::InitializeSwapChain()
    {
        std::cout << "Initializing WebGPU swap chain" << std::endl;
        wgpu::SwapChainDescriptor swapChainDesc;
        swapChainDesc.width = 640;
        swapChainDesc.height = 480;
        swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
        swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        swapChainDesc.presentMode = wgpu::PresentMode::Fifo;

        m_swapChain = m_device.CreateSwapChain(m_surface, &swapChainDesc);

        return true;
    }

    void Renderer::Render()
    {
        std::cout << "Rendering" << std::endl;
        wgpu::TextureView nextTexture = m_swapChain.GetCurrentTextureView();
        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            return;
        }

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        wgpu::RenderPassDescriptor renderPassDesc;

        wgpu::RenderPassColorAttachment renderPassColorAttachment;
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color{0.9, 0.1, 0.2, 1.0};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

        renderPass.SetPipeline(m_pipeline);
        renderPass.Draw(3);
        renderPass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor;
        cmdBufferDescriptor.label = "Command Buffer";
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

        m_device.GetQueue().Submit(1, &command);

#ifndef __EMSCRIPTEN__
        m_swapChain.Present();
        m_device.Tick();
#endif
    }

    void Renderer::Terminate()
    {
    }

}