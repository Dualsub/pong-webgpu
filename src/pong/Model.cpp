#include "pong/Model.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>

namespace pong
{
    std::unique_ptr<Model> Model::Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            std::cout << "Failed to open file: " << path << std::endl;
            return nullptr;
        }

        static_assert(sizeof(Vertex) == 9 * sizeof(float), "Vertex size is not 9 floats");

        // Read vertex data
        uint32_t vertexCount = 0;
        file.read(reinterpret_cast<char *>(&vertexCount), sizeof(vertexCount));
        std::cout << "Vertex count: " << vertexCount << std::endl;
        std::vector<Vertex> vertexData(vertexCount);
        file.read(reinterpret_cast<char *>(vertexData.data()), vertexCount * sizeof(Vertex));

        // Read index data
        uint32_t indexCount = 0;
        file.read(reinterpret_cast<char *>(&indexCount), sizeof(indexCount));
        std::cout << "Index count: " << indexCount << std::endl;
        std::vector<uint32_t> indexData(indexCount);
        file.read(reinterpret_cast<char *>(indexData.data()), indexCount * sizeof(uint32_t));

        file.close();

        // Create vertex buffer
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = vertexData.size() * sizeof(Vertex);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        wgpu::Buffer vertexBuffer = device.CreateBuffer(&bufferDesc);

        queue.WriteBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

        bufferDesc.size = indexData.size() * sizeof(uint32_t);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        wgpu::Buffer indexBuffer = device.CreateBuffer(&bufferDesc);

        // Upload geometry data to the buffer
        queue.WriteBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

        return std::make_unique<Model>(
            vertexBuffer,
            vertexCount,
            indexBuffer,
            indexCount);
    }

    std::unique_ptr<Model> Model::CreateQuad(const wgpu::Device &device, const wgpu::Queue &queue, const glm::vec2 &size, const glm::vec3 &color)
    {
        float width = size.x / 2.0f;
        float height = size.y / 2.0f;
        // Top left is origin
        std::vector<Vertex> vertices = {
            {{-width, 0.0f, -height}, {0.0f, 1.0f, 0.0f}, color},
            {{-width, 0.0f, height}, {0.0f, 1.0f, 0.0f}, color},
            {{width, 0.0f, height}, {0.0f, 1.0f, 0.0f}, color},
            {{width, 0.0f, -height}, {0.0f, 1.0f, 0.0f}, color}};
        std::vector<uint32_t> indices = {
            0, 1, 2,
            0, 2, 3};

        // Create vertex buffer
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = vertices.size() * sizeof(Vertex);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        wgpu::Buffer vertexBuffer = device.CreateBuffer(&bufferDesc);

        queue.WriteBuffer(vertexBuffer, 0, vertices.data(), bufferDesc.size);

        bufferDesc.size = indices.size() * sizeof(uint32_t);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        wgpu::Buffer indexBuffer = device.CreateBuffer(&bufferDesc);

        // Upload geometry data to the buffer
        queue.WriteBuffer(indexBuffer, 0, indices.data(), bufferDesc.size);

        return std::make_unique<Model>(
            vertexBuffer,
            vertices.size(),
            indexBuffer,
            indices.size());
    }
}