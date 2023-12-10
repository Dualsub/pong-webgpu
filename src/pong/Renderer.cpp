#include "pong/Renderer.h"

#include <webgpu/webgpu_cpp.h>

#include <cassert>
#include <iostream>
#include <vector>

namespace pong
{
    const char *shaderSource = R"(
    struct VertexInput {
        @location(0) position: vec3f,
        @location(1) color: vec3f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) color: vec3f,
    };

    struct Uniforms {
        model: mat4x4<f32>,
        view: mat4x4<f32>,
        projection: mat4x4<f32>,
        time: f32,
    };

    @group(0) @binding(0) var<uniform> uUniforms: Uniforms;

    @vertex
    fn vs_main(in: VertexInput) -> VertexOutput {
        var out: VertexOutput;
        var position = vec4f(in.position, 1.0);
        out.position = uUniforms.projection * uUniforms.view * uUniforms.model * position;
        out.color = in.color;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
        let linear_color = pow(in.color, vec3f(2.2));
        return vec4f(linear_color, 1.0);
    }

    )";

    // Helper function to round up to the next multiple of a number.
    uint32_t CeilToNextMultiple(uint32_t value, uint32_t step)
    {
        uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
        return step * divide_and_ceil;
    }

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

        if (!InitializeDepthTexture())
        {
            std::cerr << "Cannot initialize WebGPU depth texture" << std::endl;
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

        // Vertex state.
        wgpu::VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.arrayStride = sizeof(Model::Vertex);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        std::array<wgpu::VertexAttribute, 2> attributes;

        wgpu::VertexAttribute &positionAttribute = attributes[0];
        positionAttribute.shaderLocation = 0;
        positionAttribute.offset = 0;
        positionAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &colorAttribute = attributes[1];
        colorAttribute.shaderLocation = 1;
        colorAttribute.offset = sizeof(glm::vec3);
        colorAttribute.format = wgpu::VertexFormat::Float32x3;

        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;

        // Primitive state.
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
        pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

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

        // Depth testing.
        wgpu::DepthStencilState depthStencilState = {};
        depthStencilState.depthCompare = wgpu::CompareFunction::Less;
        depthStencilState.depthWriteEnabled = true;
        depthStencilState.format = c_depthFormat;

        // Deactivate the stencil alltogether
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        pipelineDesc.depthStencil = &depthStencilState;

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
        bindingLayout.buffer.hasDynamicOffset = true;

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

    bool Renderer::InitializeDepthTexture()
    {
        std::cout << "Initializing WebGPU depth texture" << std::endl;

        wgpu::TextureDescriptor textureDesc{};
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = c_depthFormat;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.size.width = 640;
        textureDesc.size.height = 480;
        textureDesc.size.depthOrArrayLayers = 1;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment;
        textureDesc.viewFormatCount = 1;
        textureDesc.viewFormats = &c_depthFormat;

        m_depthTexture = m_device.CreateTexture(&textureDesc);

        wgpu::TextureViewDescriptor textureViewDesc{};
        textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format = c_depthFormat;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;

        m_depthTextureView = m_depthTexture.CreateView(&textureViewDesc);

        return m_depthTexture != nullptr;
    }

    bool Renderer::InitializeGeometry()
    {
        std::cout << "Initializing WebGPU geometry" << std::endl;
        m_model = Model::Create(m_device, m_queue, "./dist/pong.dat");
        return m_model != nullptr;
    }

    bool Renderer::InitializeUniforms()
    {
        std::cout << "Initializing WebGPU uniforms" << std::endl;
        // Create uniform buffer
        wgpu::BufferDescriptor bufferDesc{};
        const size_t bufferStride = CeilToNextMultiple(sizeof(Uniforms), c_minUniformBufferOffsetAlignment);
        bufferDesc.size = bufferStride + sizeof(Uniforms);
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
        static uint8_t frameIndex = 0; // 0 or 1
        frameIndex = (frameIndex + 1) % 2;

        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Uniforms uniforms;
        uniforms.time = time;

        uniforms.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        const size_t uniformBufferStride = CeilToNextMultiple(sizeof(Uniforms), c_minUniformBufferOffsetAlignment);
        m_queue.WriteBuffer(m_uniformBuffer, 0, &uniforms, sizeof(Uniforms));

        // m_queue.WriteBuffer(m_uniformBuffer, uniformBufferStride, &uniforms, sizeof(Uniforms));

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

        wgpu::RenderPassDepthStencilAttachment renderPassDepthStencilAttachment;
        renderPassDepthStencilAttachment.view = m_depthTextureView;
        renderPassDepthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        renderPassDepthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
        renderPassDepthStencilAttachment.depthClearValue = 1.0f;
        renderPassDepthStencilAttachment.depthReadOnly = false;

        // Stencil is not used
        renderPassDepthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
        renderPassDepthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
        renderPassDepthStencilAttachment.stencilClearValue = 0;
        renderPassDepthStencilAttachment.stencilReadOnly = true;

        renderPassDesc.depthStencilAttachment = &renderPassDepthStencilAttachment;

        renderPassDesc.timestampWrites = nullptr;
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

        renderPass.SetPipeline(m_pipeline);

        size_t vertexCount = m_model->GetVertexCount();
        renderPass.SetVertexBuffer(0, m_model->GetVertexBuffer(), 0, vertexCount * 5 * sizeof(float));

        size_t indexCount = m_model->GetIndexCount();
        renderPass.SetIndexBuffer(m_model->GetIndexBuffer(), wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t));

        uint32_t dynamicOffset = 0;

        renderPass.SetBindGroup(0, m_bindGroup, 1, &dynamicOffset);
        renderPass.DrawIndexed(indexCount);

        // dynamicOffset = static_cast<uint32_t>(uniformBufferStride);
        // renderPass.SetBindGroup(0, m_bindGroup, 1, &dynamicOffset);
        // renderPass.DrawIndexed(indexCount);

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