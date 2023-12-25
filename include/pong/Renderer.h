#pragma once

#include "pong/Device.h"
#include "pong/Model.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <webgpu/webgpu_cpp.h>

#include <memory>
#include <vector>

namespace pong
{
    struct RenderBatch
    {
        Model *model;
        std::vector<glm::mat4> transforms;
    };

    class Renderer
    {
    private:
        struct Uniforms
        {
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 view = glm::lookAt(glm::vec3(200.0f, 300.0f, 200.0f), // Camera position in World Space
                                         glm::vec3(200.0f, 0.0, 150.0f),    // and looks at the origin
                                         glm::vec3(0.0f, 1.0f, 0.0f));      // Head is up
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(c_width) / float(c_height), 0.1f, 10000.0f);
            glm::vec4 light = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
            float time;
            float _padding[3];
        };
        // Check alignment
        static_assert(sizeof(Uniforms) % 16 == 0);

        // Constants
        const wgpu::TextureFormat c_swapChainFormat = wgpu::TextureFormat::BGRA8Unorm;
        const wgpu::TextureFormat c_depthFormat = wgpu::TextureFormat::Depth24Plus;
        const size_t c_minUniformBufferOffsetAlignment = 64;
        const static uint32_t c_width = 1060;
        const static uint32_t c_height = 600;
        const std::string c_canvasSelector = "#canvas";

        // Window
        uint32_t m_width = c_width;
        uint32_t m_height = c_height;

        // Buffers
        wgpu::VertexBufferLayout m_vertexBufferLayout;

        // Device
        wgpu::Instance m_instance = {};
        wgpu::Device m_device = {};
        wgpu::Surface m_surface = {};
        wgpu::Queue m_queue = {};

        // Pipeline
        wgpu::RenderPipeline m_pipeline = {};
        wgpu::RenderPipeline m_shadowPipeline = {};

        // Swap chain
        wgpu::SwapChain m_swapChain = {};

        // Shader
        wgpu::ShaderModule m_shaderModule = {};

        // Bind group
        wgpu::BindGroupLayout m_bindGroupLayout = {};
        wgpu::BindGroup m_bindGroup = {};

        // Depth texture
        wgpu::Texture m_depthTexture = {};
        wgpu::TextureView m_depthTextureView = {};

        wgpu::Texture m_shadowDepthTexture = {};
        wgpu::TextureView m_shadowDepthTextureView = {};
        wgpu::Sampler m_shadowDepthSampler = {};

        // Uniforms
        wgpu::Buffer m_uniformBuffer = {};
        Uniforms m_uniforms = {};

        // Batches
        std::vector<RenderBatch> m_batches;

        bool InitializeSurface();
        bool InitializeSwapChain();
        bool InitializeBindGroupLayout();
        bool InitializeShadowPipeline();
        bool InitializePipeline();
        bool InitializeDepthTexture();
        bool InitializeGeometry();
        bool InitializeUniforms();
        bool InitializeBindGroup();

        void RenderBatches(wgpu::RenderPassEncoder &pass);

    public:
        Renderer() {}
        ~Renderer() {}

        void Run(void (*mainLoopCallback)(void));

        bool Initialize(const DeviceContext &context);

        void OnResize(uint32_t width, uint32_t height);

        void SubmitInstances(Model *model, const std::vector<glm::mat4> &transforms)
        {
            m_batches.push_back({model, transforms});
        }

        void SetCameraView(const glm::mat4 &view)
        {
            m_uniforms.view = view;
        }

        void Render();
        void Tick();
        void Terminate();

        // Should be moved in the future
        std::unique_ptr<Model> CreateModel(const std::string &path) const { return Model::Create(m_device, m_queue, path); }
        std::unique_ptr<Model> CreateQuad(const glm::vec2 &size, const glm::vec3 &color) const { return Model::CreateQuad(m_device, m_queue, size, color); }
    };
}