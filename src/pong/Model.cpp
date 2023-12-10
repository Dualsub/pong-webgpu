#include "pong/Model.h"

#include <array>

namespace pong
{
    std::unique_ptr<Model> Model::Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path)
    {
        // clang-format off
        std::array<float, 30> vertexData = {
            // x,   y,     r,   g,   b
            -0.5, -0.5,   1.0, 0.0, 0.0,
            +0.5, -0.5,   0.0, 1.0, 0.0,
            +0.5, +0.5,   0.0, 0.0, 1.0,
            -0.5, +0.5,   1.0, 1.0, 0.0
        };
        // clang-format on
        int vertexCount = static_cast<int>(vertexData.size() / 5);

        // Create vertex buffer
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = vertexData.size() * sizeof(float);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        wgpu::Buffer vertexBuffer = device.CreateBuffer(&bufferDesc);

        queue.WriteBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

        // Create index buffer
        std::array<uint16_t, 6> indexData = {
            0, 1, 2, // Triangle #0
            0, 2, 3  // Triangle #1
        };

        bufferDesc.size = indexData.size() * sizeof(uint16_t);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        wgpu::Buffer indexBuffer = device.CreateBuffer(&bufferDesc);

        // Upload geometry data to the buffer
        queue.WriteBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

        return std::make_unique<Model>(
            vertexBuffer,
            vertexData.size() / 5,
            indexBuffer,
            indexData.size());
    }
}