#include "pong/Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include <webgpu/webgpu_cpp.h>
#include <emscripten/emscripten.h>

#include <cassert>
#include <iostream>
#include <vector>

namespace pong
{
    const char *shaderSource = R"(
    struct VertexInput {
        @location(0) position: vec3f,
        @location(1) normal: vec3f,
        @location(2) color: vec3f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) color: vec3f,
        @location(1) normal: vec3f,
        @location(2) fragPosLightSpace: vec3f,
    };

    struct Uniforms {
        model: mat4x4<f32>,
        view: mat4x4<f32>,
        projection: mat4x4<f32>,
        lightViewProjection: mat4x4<f32>,
        lightDirection: vec3<f32>,
        cameraPosition: vec3<f32>,
        time: f32,
    };

    @group(0) @binding(0) var<uniform> uUniforms: Uniforms;
    @group(1) @binding(0) var shadowMap: texture_depth_2d;
    @group(1) @binding(1) var shadowSampler: sampler_comparison;

    @vertex
    fn vs_main(in: VertexInput) -> VertexOutput {
        var out: VertexOutput;
        var position = vec4f(in.position, 1.0);
        out.position = uUniforms.projection * uUniforms.view * uUniforms.model * position;

        let posFromLight = uUniforms.lightViewProjection * uUniforms.model * position;
        out.fragPosLightSpace = vec3f(
            posFromLight.xy * vec2f(0.5, -0.5) + vec2f(0.5),
            posFromLight.z
        );

        out.color = in.color;
        out.normal = in.normal;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
        var visibility = 0.0;
        for (var y = -1; y <= 1; y++) {
            for (var x = -1; x <= 1; x++) {
                let offset = vec2<f32>(vec2(x, y)) * 1.0 / 2048.0;
                visibility += textureSampleCompare(
                    shadowMap, shadowSampler,
                    in.fragPosLightSpace.xy + offset, in.fragPosLightSpace.z - 0.007
                );
            }
        }
        visibility /= 9.0;

        // vector to point from the light source towards the fragment's position
        let lightDir = normalize(-uUniforms.lightDirection);
        let viewDir = normalize(uUniforms.cameraPosition - in.position.xyz);

        // diffuse shading
        let diff = max(dot(in.normal, lightDir), 0.0);
        
        // specular shading
        let halfWay = normalize(lightDir + viewDir);
        
        let spec = pow(max(dot(halfWay, in.normal), 0.0), 0.3);
        let specular = vec3f(spec);
        
        let diffuse = diff * in.color; 
        let ambient = 0.2 * in.color;

        // let linear_color = pow(in.color, vec3f(2.2));
        let light = dot(normalize(in.normal), lightDir * 0.5 + 0.5) * visibility;
        return vec4f(ambient + light * in.color * 0.8, 1.0);
    }

    )";

    const char *shadowShaderSource = R"(
 
    struct Uniforms {
        model: mat4x4<f32>,
        view: mat4x4<f32>,
        projection: mat4x4<f32>,
        lightViewProjection: mat4x4<f32>,
        lightDirection: vec3<f32>,
        cameraPosition: vec3<f32>,
        time: f32,
    };

    @group(0) @binding(0) var<uniform> uUniforms: Uniforms;
        
    @vertex
    fn vs_main(@location(0) position: vec3<f32>) -> @builtin(position) vec4<f32> {
        // Output triangle position in clip space
        return uUniforms.lightViewProjection * uUniforms.model * vec4(position, 1.0);
    }

    )";

    const char *spriteShaderSource = R"(
    struct VertexInput {
        @builtin(instance_index) instanceIndex: u32,
        @location(0) position: vec3f,
        @location(1) texCoord: vec2f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) texCoord: vec2f,
    };

    struct SpriteUniforms {
        model: mat4x4<f32>,
        view: mat4x4<f32>,
        projection: mat4x4<f32>,
        offsetAndSize: vec4<f32>,
    };

    struct SpriteInstance {
        transform: mat4x4<f32>,
        offsetAndSize: vec4<f32>,
    }

    @group(0) @binding(0) var spriteTexture: texture_2d<f32>;
    @group(0) @binding(1) var spriteSampler: sampler;
    @group(0) @binding(2) var<uniform> uUniforms: SpriteUniforms;
    @group(0) @binding(3) var<storage, read> spriteInstanceBuffer: array<SpriteInstance>;

    @vertex
    fn vs_main(in: VertexInput) -> VertexOutput {
        var out: VertexOutput;
        let instance = spriteInstanceBuffer[in.instanceIndex];
        var position = vec4f(in.position, 1.0);
        out.position = uUniforms.projection * uUniforms.view * instance.transform * position;
        out.texCoord = in.texCoord * instance.offsetAndSize.zw + instance.offsetAndSize.xy;
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
        return textureSample(spriteTexture, spriteSampler, in.texCoord);
    }

    )";

    // Helper function to round up to the next multiple of a number.
    uint32_t CeilToNextMultiple(uint32_t value, uint32_t step)
    {
        uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
        return step * divide_and_ceil;
    }

    bool Renderer::Initialize(const DeviceContext &context, uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
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

        if (!InitializeBindGroupLayout())
        {
            std::cerr << "Cannot initialize WebGPU bind group layout" << std::endl;
            return false;
        }

        if (!InitializeShadowPipeline())
        {
            std::cerr << "Cannot initialize WebGPU shadow pipeline" << std::endl;
            return false;
        }

        if (!InitializeRenderPipeline())
        {
            std::cerr << "Cannot initialize WebGPU pipeline" << std::endl;
            return false;
        }

        if (!InitializeSpritePipeline())
        {
            std::cerr << "Cannot initialize WebGPU sprite pipeline" << std::endl;
            return false;
        }

        if (!InitializeDepthTexture())
        {
            std::cerr << "Cannot initialize WebGPU depth texture" << std::endl;
            return false;
        }

        if (!InitializeShadowMapTexture())
        {
            std::cerr << "Cannot initialize WebGPU shadow map texture" << std::endl;
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
        canvasDesc.selector = c_canvasSelector.c_str();

        wgpu::SurfaceDescriptor surfaceDesc;
        surfaceDesc.nextInChain = &canvasDesc;
        m_surface = m_instance.CreateSurface(&surfaceDesc);

        return m_surface != nullptr;
    }

    bool Renderer::InitializeSwapChain()
    {
        std::cout << "Initializing WebGPU swap chain" << std::endl;
        wgpu::SwapChainDescriptor swapChainDesc;
        swapChainDesc.width = m_width;
        swapChainDesc.height = m_height;
        swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
        swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        swapChainDesc.presentMode = wgpu::PresentMode::Fifo;

        m_swapChain = m_device.CreateSwapChain(m_surface, &swapChainDesc);

        return m_swapChain != nullptr;
    }

    bool Renderer::InitializeBindGroupLayout()
    {
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
        m_bindGroupLayouts[0] = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // Shadow binding layout.
        std::array<wgpu::BindGroupLayoutEntry, 2> bindingLayouts = {};
        bindingLayouts[0].binding = 0;
        bindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        bindingLayouts[0].texture.sampleType = wgpu::TextureSampleType::Depth;
        bindingLayouts[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        bindingLayouts[1].binding = 1;
        bindingLayouts[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        bindingLayouts[1].sampler.type = wgpu::SamplerBindingType::Comparison;

        bindGroupLayoutDesc.entryCount = bindingLayouts.size();
        bindGroupLayoutDesc.entries = bindingLayouts.data();

        m_bindGroupLayouts[1] = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // Sprite binding layout.
        std::array<wgpu::BindGroupLayoutEntry, 4> spriteBindingLayouts = {};
        spriteBindingLayouts[0].binding = 0;
        spriteBindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        spriteBindingLayouts[0].texture.sampleType = wgpu::TextureSampleType::Float;
        spriteBindingLayouts[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        spriteBindingLayouts[1].binding = 1;
        spriteBindingLayouts[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        spriteBindingLayouts[1].sampler.type = wgpu::SamplerBindingType::Filtering;

        spriteBindingLayouts[2].binding = 2;
        spriteBindingLayouts[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        spriteBindingLayouts[2].buffer.type = wgpu::BufferBindingType::Uniform;
        spriteBindingLayouts[2].buffer.minBindingSize = sizeof(SpriteUniforms);
        spriteBindingLayouts[2].buffer.hasDynamicOffset = false;

        spriteBindingLayouts[3].binding = 3;
        spriteBindingLayouts[3].visibility = wgpu::ShaderStage::Vertex;
        spriteBindingLayouts[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        spriteBindingLayouts[3].buffer.minBindingSize = sizeof(SpriteBatch::Instance) * c_maxSprites;
        spriteBindingLayouts[3].buffer.hasDynamicOffset = false;

        bindGroupLayoutDesc.entryCount = spriteBindingLayouts.size();
        bindGroupLayoutDesc.entries = spriteBindingLayouts.data();

        m_bindGroupLayouts[2] = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        for (auto &&layout : m_bindGroupLayouts)
        {
            if (layout == nullptr)
            {
                return false;
            }
        }

        return true;
    }

    bool Renderer::InitializeShadowPipeline()
    {
        std::cout << "Initializing WebGPU shadow pipeline" << std::endl;
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
        shaderCodeDesc.code = shadowShaderSource;
        m_shaderModule = m_device.CreateShaderModule(&shaderDesc);

        // Vertex state.
        wgpu::VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.arrayStride = sizeof(Model::Vertex);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        std::array<wgpu::VertexAttribute, 3> attributes;

        wgpu::VertexAttribute &positionAttribute = attributes[0];
        positionAttribute.shaderLocation = 0;
        positionAttribute.offset = 0;
        positionAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &normalAttribute = attributes[1];
        normalAttribute.shaderLocation = 1;
        normalAttribute.offset = sizeof(glm::vec3);
        normalAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &colorAttribute = attributes[2];
        colorAttribute.shaderLocation = 2;
        colorAttribute.offset = sizeof(glm::vec3) * 2;
        colorAttribute.format = wgpu::VertexFormat::Float32x3;

        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        pipelineDesc.vertex.module = m_shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;

        // Primitive state.
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
        pipelineDesc.primitive.cullMode = wgpu::CullMode::Front;

        // Depth testing.
        wgpu::DepthStencilState depthStencilState = {};
        depthStencilState.depthCompare = wgpu::CompareFunction::Less;
        depthStencilState.depthWriteEnabled = true;
        depthStencilState.format = c_shadowMapDepthFormat;

        // Deactivate the stencil alltogether
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        pipelineDesc.depthStencil = &depthStencilState;

        // Create the pipeline layout
        wgpu::PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 1;
        layoutDesc.bindGroupLayouts = m_bindGroupLayouts.data();
        wgpu::PipelineLayout layout = m_device.CreatePipelineLayout(&layoutDesc);
        pipelineDesc.layout = layout;

        // Create the pipeline.
        m_shadowPipeline = m_device.CreateRenderPipeline(&pipelineDesc);

        return m_shadowPipeline != nullptr;
    }

    bool Renderer::InitializeSpritePipeline()
    {
        std::cout << "Initializing WebGPU sprite pipeline" << std::endl;
        wgpu::RenderPipelineDescriptor pipelineDesc = {};

        // Shader source.
        wgpu::ShaderModuleDescriptor shaderDesc;
        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        // Set the chained struct's header
        shaderCodeDesc.nextInChain = nullptr;
        shaderCodeDesc.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        // Connect the chain
        shaderDesc.nextInChain = &shaderCodeDesc;
        shaderCodeDesc.code = spriteShaderSource;
        m_shaderModule = m_device.CreateShaderModule(&shaderDesc);

        // Vertex state.
        wgpu::VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.arrayStride = sizeof(Model::SpriteVertex);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        std::array<wgpu::VertexAttribute, 2> attributes;

        wgpu::VertexAttribute &positionAttribute = attributes[0];
        positionAttribute.shaderLocation = 0;
        positionAttribute.offset = 0;
        positionAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &texCoordAttribute = attributes[1];
        texCoordAttribute.shaderLocation = 1;
        texCoordAttribute.offset = sizeof(glm::vec3);
        texCoordAttribute.format = wgpu::VertexFormat::Float32x2;

        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        pipelineDesc.vertex.module = m_shaderModule;
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
        fragmentState.module = m_shaderModule;
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

        // Create the pipeline layout
        wgpu::PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 1;
        layoutDesc.bindGroupLayouts = &m_bindGroupLayouts[2];
        wgpu::PipelineLayout layout = m_device.CreatePipelineLayout(&layoutDesc);
        pipelineDesc.layout = layout;

        // Create the pipeline.
        m_spritePipeline = m_device.CreateRenderPipeline(&pipelineDesc);

        return m_spritePipeline != nullptr;
    }

    bool Renderer::InitializeRenderPipeline()
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
        m_shaderModule = m_device.CreateShaderModule(&shaderDesc);

        // Vertex state.
        wgpu::VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.arrayStride = sizeof(Model::Vertex);
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

        std::array<wgpu::VertexAttribute, 3> attributes;

        wgpu::VertexAttribute &positionAttribute = attributes[0];
        positionAttribute.shaderLocation = 0;
        positionAttribute.offset = 0;
        positionAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &normalAttribute = attributes[1];
        normalAttribute.shaderLocation = 1;
        normalAttribute.offset = sizeof(glm::vec3);
        normalAttribute.format = wgpu::VertexFormat::Float32x3;

        wgpu::VertexAttribute &colorAttribute = attributes[2];
        colorAttribute.shaderLocation = 2;
        colorAttribute.offset = sizeof(glm::vec3) * 2;
        colorAttribute.format = wgpu::VertexFormat::Float32x3;

        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        pipelineDesc.vertex.module = m_shaderModule;
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
        fragmentState.module = m_shaderModule;
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

        // Create the pipeline layout
        wgpu::PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 2;
        layoutDesc.bindGroupLayouts = m_bindGroupLayouts.data();
        wgpu::PipelineLayout layout = m_device.CreatePipelineLayout(&layoutDesc);
        pipelineDesc.layout = layout;

        // Create the pipeline.
        m_renderPipeline = m_device.CreateRenderPipeline(&pipelineDesc);

        return m_renderPipeline != nullptr;
    }

    bool Renderer::InitializeDepthTexture()
    {
        // std::cout << "Initializing WebGPU depth texture" << std::endl;

        wgpu::TextureDescriptor textureDesc{};
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = c_depthFormat;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.size.width = m_width;
        textureDesc.size.height = m_height;
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

    bool Renderer::InitializeShadowMapTexture()
    {
        wgpu::TextureDescriptor textureDesc{};
        textureDesc.label = "Shadow Map Texture";
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = c_shadowMapDepthFormat;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.size.width = c_shadowMapSize;
        textureDesc.size.height = c_shadowMapSize;
        textureDesc.size.depthOrArrayLayers = 1;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
        textureDesc.viewFormatCount = 1;
        textureDesc.viewFormats = &c_shadowMapDepthFormat;

        m_shadowDepthTexture = m_device.CreateTexture(&textureDesc);

        wgpu::TextureViewDescriptor textureViewDesc{};
        textureViewDesc.label = "Shadow Map Texture View";
        textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDesc.format = c_shadowMapDepthFormat;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;

        m_shadowDepthTextureView = m_shadowDepthTexture.CreateView(&textureViewDesc);

        wgpu::SamplerDescriptor samplerDesc = {};
        samplerDesc.compare = wgpu::CompareFunction::Less;
        m_shadowDepthSampler = m_device.CreateSampler(&samplerDesc);

        return m_depthTexture != nullptr && m_shadowDepthTextureView != nullptr && m_shadowDepthSampler != nullptr;
    }

    bool Renderer::InitializeGeometry()
    {
        std::cout << "Initializing WebGPU geometry" << std::endl;
        m_quad = Model::CreateSpriteQuad(m_device, m_queue);
        return m_quad != nullptr;
    }

    bool Renderer::InitializeUniforms()
    {
        std::cout << "Initializing WebGPU uniforms" << std::endl;
        // Create uniform buffer
        wgpu::BufferDescriptor bufferDesc{};
        const size_t bufferStride = CeilToNextMultiple(sizeof(Uniforms), c_minUniformBufferOffsetAlignment);
        bufferDesc.size = 5 * bufferStride;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        bufferDesc.mappedAtCreation = false;

        m_uniformBuffer = m_device.CreateBuffer(&bufferDesc);
        m_uniforms.lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        m_uniforms.projection = glm::perspective(glm::radians(52.5f), float(m_width) / float(m_height), 0.1f, 1000.0f);

        wgpu::BufferDescriptor spriteBufferDesc{};
        const size_t spriteBufferStride = CeilToNextMultiple(sizeof(SpriteUniforms), c_minUniformBufferOffsetAlignment);
        spriteBufferDesc.size = 5 * spriteBufferStride;
        spriteBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        spriteBufferDesc.mappedAtCreation = false;

        m_spriteUniformBuffer = m_device.CreateBuffer(&spriteBufferDesc);

        wgpu::BufferDescriptor instanceBufferDesc{};
        instanceBufferDesc.size = c_maxSprites * sizeof(SpriteBatch::Instance);
        instanceBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;
        instanceBufferDesc.mappedAtCreation = false;

        m_spriteInstanceBuffer = m_device.CreateBuffer(&instanceBufferDesc);

        return m_uniformBuffer != nullptr && m_spriteUniformBuffer != nullptr;
    }

    bool Renderer::InitializeBindGroup()
    {
        // Create a binding
        wgpu::BindGroupEntry uniformBinding = {};
        uniformBinding.binding = 0;
        uniformBinding.buffer = m_uniformBuffer;
        uniformBinding.size = sizeof(Uniforms);

        // A bind group contains one or multiple bindings
        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = m_bindGroupLayouts[0];
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &uniformBinding;
        std::cout << "hello" << std::endl;
        m_bindGroup = m_device.CreateBindGroup(&bindGroupDesc);

        std::array<wgpu::BindGroupEntry, 2> shadowMapBindings{};
        shadowMapBindings[0].binding = 0;
        shadowMapBindings[0].textureView = m_shadowDepthTextureView;

        shadowMapBindings[1].binding = 1;
        shadowMapBindings[1].sampler = m_shadowDepthSampler;

        bindGroupDesc.layout = m_bindGroupLayouts[1];
        bindGroupDesc.entryCount = shadowMapBindings.size();
        bindGroupDesc.entries = shadowMapBindings.data();
        m_shadowBindGroup = m_device.CreateBindGroup(&bindGroupDesc);

        return m_bindGroup != nullptr && m_shadowBindGroup != nullptr;
    }

    void Renderer::AddSpriteBindGroup(Texture *texture)
    {
        if (!texture)
        {
            return;
        }

        std::array<wgpu::BindGroupEntry, 4> spriteBindings = {};
        spriteBindings[0].binding = 0;
        spriteBindings[0].textureView = texture->GetTextureView();

        spriteBindings[1].binding = 1;
        spriteBindings[1].sampler = texture->GetSampler();

        spriteBindings[2].binding = 2;
        spriteBindings[2].buffer = m_spriteUniformBuffer;
        spriteBindings[2].size = sizeof(SpriteUniforms);

        spriteBindings[3].binding = 3;
        spriteBindings[3].buffer = m_spriteInstanceBuffer;
        spriteBindings[3].size = c_maxSprites * sizeof(SpriteBatch::Instance);

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = m_bindGroupLayouts[2];
        bindGroupDesc.entryCount = spriteBindings.size();
        bindGroupDesc.entries = spriteBindings.data();

        m_spriteBindGroups.emplace(texture->GetId(), m_device.CreateBindGroup(&bindGroupDesc));
    }

    void Renderer::RenderBatches(wgpu::RenderPassEncoder &pass)
    {
        static const size_t uniformBufferStride = CeilToNextMultiple(sizeof(Uniforms), c_minUniformBufferOffsetAlignment);

        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Uniforms uniforms = m_uniforms;
        uniforms.time = time;

        uint32_t index = 0;
        for (auto &&batch : m_batches)
        {
            Model *model = batch.model;
            if (model == nullptr)
            {
                continue;
            }

            size_t vertexCount = model->GetVertexCount();
            pass.SetVertexBuffer(0, model->GetVertexBuffer(), 0, vertexCount * sizeof(Model::Vertex));

            size_t indexCount = model->GetIndexCount();
            pass.SetIndexBuffer(model->GetIndexBuffer(), wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t));

            for (auto &&transform : batch.transforms)
            {
                uint32_t dynamicOffset = index * uniformBufferStride;
                uniforms.model = transform;
                m_queue.WriteBuffer(m_uniformBuffer, dynamicOffset, &uniforms, sizeof(Uniforms));
                pass.SetBindGroup(0, m_bindGroup, 1, &dynamicOffset);
                pass.DrawIndexed(indexCount);
                index++;
            }
        }
    }

    void Renderer::RenderSpriteBatches(wgpu::RenderPassEncoder &pass)
    {
        m_spriteUniforms.projection = m_uniforms.projection;
        m_spriteUniforms.view = m_uniforms.view;
        SpriteUniforms uniforms = m_spriteUniforms;

        m_queue.WriteBuffer(m_spriteUniformBuffer, 0, &uniforms, sizeof(SpriteUniforms));

        size_t vertexCount = m_quad->GetVertexCount();
        pass.SetVertexBuffer(0, m_quad->GetVertexBuffer(), 0, vertexCount * sizeof(Model::SpriteVertex));

        size_t indexCount = m_quad->GetIndexCount();
        pass.SetIndexBuffer(m_quad->GetIndexBuffer(), wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t));

        uint32_t index = 0;
        for (auto &&batch : m_spriteBatches)
        {
            if (batch.texture == nullptr || batch.texture->GetId() == 0 || m_spriteBindGroups.find(batch.texture->GetId()) == m_spriteBindGroups.end() || batch.instances.empty())
            {
                continue;
            }

            wgpu::BindGroup &bindGroup = m_spriteBindGroups[batch.texture->GetId()];
            pass.SetBindGroup(0, bindGroup, 0, nullptr);

            m_queue.WriteBuffer(m_spriteInstanceBuffer, 0, batch.instances.data(), batch.instances.size() * sizeof(SpriteBatch::Instance));

            pass.DrawIndexed(indexCount, batch.instances.size());
        }

        m_spriteBatches.clear();
    }

    void Renderer::Resize(uint32_t width, uint32_t height)
    {
        std::cout << "Resizing WebGPU surface to " << width << "x" << height << std::endl;
        m_width = width;
        m_height = height;

        m_swapChain.Release();
        InitializeSwapChain();

        m_depthTexture.Release();
        m_depthTextureView.Release();
        InitializeDepthTexture();

        m_uniforms.projection = glm::perspective(glm::radians(52.5f), float(m_width) / float(m_height), 0.1f, 1000.0f);
    }

    void Renderer::Render()
    {
        wgpu::TextureView nextTexture = m_swapChain.GetCurrentTextureView();
        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            return;
        }

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        { // Shadow pass
            wgpu::RenderPassDescriptor shadowPassDesc;

            shadowPassDesc.colorAttachmentCount = 0;
            shadowPassDesc.colorAttachments = nullptr;

            wgpu::RenderPassDepthStencilAttachment renderPassDepthStencilAttachment;
            renderPassDepthStencilAttachment.view = m_shadowDepthTextureView;
            renderPassDepthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
            renderPassDepthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
            renderPassDepthStencilAttachment.depthClearValue = 1.0f;
            renderPassDepthStencilAttachment.depthReadOnly = false;

            // Stencil is not used
            renderPassDepthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
            renderPassDepthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
            renderPassDepthStencilAttachment.stencilClearValue = 0;
            renderPassDepthStencilAttachment.stencilReadOnly = true;

            shadowPassDesc.depthStencilAttachment = &renderPassDepthStencilAttachment;

            shadowPassDesc.timestampWrites = nullptr;
            wgpu::RenderPassEncoder shadowPass = encoder.BeginRenderPass(&shadowPassDesc);

            shadowPass.SetPipeline(m_shadowPipeline);

            RenderBatches(shadowPass);

            shadowPass.End();
        }

        { // Render pass

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

            renderPass.SetPipeline(m_renderPipeline);
            renderPass.SetBindGroup(1, m_shadowBindGroup);
            RenderBatches(renderPass);

            renderPass.SetPipeline(m_spritePipeline);
            RenderSpriteBatches(renderPass);

            renderPass.End();
        }

        wgpu::CommandBuffer command = encoder.Finish();
        m_queue.Submit(1, &command);

#ifndef __EMSCRIPTEN__
        m_swapChain.Present();
        m_device.Tick();
#endif

        m_batches.clear();
    }

    void Renderer::Tick()
    {
#ifdef __EMSCRIPTEN__
        m_queue.Submit(0, nullptr);
#else
        // m_device.Tick();
#endif
    }

    void Renderer::Terminate()
    {
    }
}