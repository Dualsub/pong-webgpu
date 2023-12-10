#pragma once

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
        Model() {}
        Model(const wgpu::Buffer &vertexBuffer, size_t vertexCount, const wgpu::Buffer &indexBuffer, size_t indexCount)
            : m_vertexBuffer(vertexBuffer), m_vertexCount(vertexCount), m_indexBuffer(indexBuffer), m_indexCount(indexCount) {}
        ~Model() {}

        const wgpu::Buffer &GetVertexBuffer() { return m_vertexBuffer; }
        const wgpu::Buffer &GetIndexBuffer() { return m_indexBuffer; }
        size_t GetVertexCount() { return m_vertexCount; }
        size_t GetIndexCount() { return m_indexCount; }

        static std::unique_ptr<Model> Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path);
    };
}
