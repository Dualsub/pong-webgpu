#include "pong/Renderer.h"

#include <webgpu/webgpu_cpp.h>

#include <cassert>
#include <iostream>
#include <vector>

namespace pong
{
    const char *shaderSource = R"(
    struct VertexInput {
        @location(0) position: vec2f,
        @location(1) color: vec3f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) color: vec3f,
    };

    struct Uniforms {
        time: f32,
    };

    @group(0) @binding(0) var<uniform> uUniforms: Uniforms;

    @vertex
    fn vs_main(in: VertexInput) -> VertexOutput {
        var out: VertexOutput;
        let ratio = 640.0 / 480.0;
        var offset = vec2f(-0.6875, -0.463);
        offset += 0.3 * vec2f(cos(uUniforms.time), sin(uUniforms.time));
        out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
        out.color = in.color;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
        let linear_color = pow(in.color, vec3f(2.2));
        return vec4f(linear_color, 1.0);
    }

    )";

    bool Renderer::Initialize(const DeviceContext &context)
    {
        m_instance = context.instance;
        m_device = context.device;
        m_queue = m_device.GetQueue();

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

        if (!InitializeGeometry())
        {
            std::cerr << "Cannot initialize WebGPU geometry" << std::endl;
            return false;
        }

        if (!InitializeUniforms())
        {
            std::cerr << "Cannot initialize WebGPU uniforms" << std::endl;
            return false;
        }

        if (!InitializeBindGroup())
        {
            std::cerr << "Cannot initialize WebGPU bind group" << std::endl;
            return false;
        }

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
        wgpu::VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.arrayStride = 5 * sizeof(float);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        std::array<wgpu::VertexAttribute, 2> attributes;

        wgpu::VertexAttribute &positionAttribute = attributes[0];
        positionAttribute.shaderLocation = 0;
        positionAttribute.offset = 0;
        positionAttribute.format = wgpu::VertexFormat::Float32x2;

        wgpu::VertexAttribute &colorAttribute = attributes[1];
        colorAttribute.shaderLocation = 1;
        colorAttribute.offset = 2 * sizeof(float);
        colorAttribute.format = wgpu::VertexFormat::Float32x3;

        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

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

        // Binding layout.

        wgpu::BindGroupLayoutEntry bindingLayout = {};
        bindingLayout.binding = 0;
        bindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
        bindingLayout.buffer.minBindingSize = sizeof(Uniforms);

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &bindingLayout;
        m_bindGroupLayout = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // Create the pipeline layout
        wgpu::PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 1;
        layoutDesc.bindGroupLayouts = &m_bindGroupLayout;
        wgpu::PipelineLayout layout = m_device.CreatePipelineLayout(&layoutDesc);
        pipelineDesc.layout = layout;

        // Create the pipeline.
        m_pipeline = m_device.CreateRenderPipeline(&pipelineDesc);
        std::cout << "  Initializing WebGPU pipeline" << std::endl;

        return m_pipeline != nullptr;
    }

    bool Renderer::InitializeGeometry()
    {
        m_model = Model::Create(m_device, m_queue, "assets/models/pong.obj");
        return m_model != nullptr;
    }

    bool Renderer::InitializeUniforms()
    {
        // Create uniform buffer
        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.size = sizeof(Uniforms);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        bufferDesc.mappedAtCreation = false;

        m_uniformBuffer = m_device.CreateBuffer(&bufferDesc);

        return m_uniformBuffer != nullptr;
    }

    bool Renderer::InitializeBindGroup()
    {
        // Create a binding
        wgpu::BindGroupEntry binding{};
        binding.binding = 0;
        binding.buffer = m_uniformBuffer;
        binding.size = sizeof(Uniforms);

        // A bind group contains one or multiple bindings
        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = m_bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;
        m_bindGroup = m_device.CreateBindGroup(&bindGroupDesc);

        return m_bindGroup != nullptr;
    }

    void Renderer::Render()
    {
        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        m_queue.WriteBuffer(m_uniformBuffer, 0, &time, sizeof(Uniforms));

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
        renderPassColorAttachment.clearValue = wgpu::Color{0.05, 0.05, 0.05, 1.0};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

        renderPass.SetPipeline(m_pipeline);

        size_t vertexCount = m_model->GetVertexCount();
        renderPass.SetVertexBuffer(0, m_model->GetVertexBuffer(), 0, vertexCount * 5 * sizeof(float));

        size_t indexCount = m_model->GetIndexCount();
        renderPass.SetIndexBuffer(m_model->GetIndexBuffer(), wgpu::IndexFormat::Uint16, 0, indexCount * sizeof(uint16_t));

        renderPass.SetBindGroup(0, m_bindGroup);

        renderPass.DrawIndexed(indexCount);

        renderPass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor;
        cmdBufferDescriptor.label = "Command Buffer";
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

        m_queue.Submit(1, &command);

#ifndef __EMSCRIPTEN__
        m_swapChain.Present();
        m_device.Tick();
#endif
    }

    void Renderer::Tick()
    {
#ifdef __EMSCRIPTEN__
        m_queue.Submit(0, nullptr);
#else
        m_device.Tick();
#endif
    }

    void Renderer::Terminate()
    {
    }

}