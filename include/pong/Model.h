#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <string>

namespace pong
{
    class Model
    {
    private:
        wgpu::Buffer m_vertexBuffer;
        wgpu::Buffer m_indexBuffer;
        size_t m_vertexCount;
        size_t m_indexCount;

    public:
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec3 color;
        };

        struct SpriteVertex
        {
            glm::vec3 position;
            glm::vec2 texCoord;
        };

        Model() {}
        Model(const wgpu::Buffer &vertexBuffer, size_t vertexCount, const wgpu::Buffer &indexBuffer, size_t indexCount)
            : m_vertexBuffer(vertexBuffer), m_vertexCount(vertexCount), m_indexBuffer(indexBuffer), m_indexCount(indexCount) {}
        ~Model() {}

        const wgpu::Buffer &GetVertexBuffer() { return m_vertexBuffer; }
        const wgpu::Buffer &GetIndexBuffer() { return m_indexBuffer; }
        size_t GetVertexCount() { return m_vertexCount; }
        size_t GetIndexCount() { return m_indexCount; }

        static std::unique_ptr<Model> Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path);
        static std::unique_ptr<Model> CreateQuad(const wgpu::Device &device, const wgpu::Queue &queue, const glm::vec2 &size, const glm::vec3 &color);
        static std::unique_ptr<Model> CreateSpriteQuad(const wgpu::Device &device, const wgpu::Queue &queue);
    };
}
