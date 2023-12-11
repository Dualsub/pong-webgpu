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
            // std::cout << "Failed to open file: " << path << std::endl;
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
            vertexData.size() / 5,
            indexBuffer,
            indexData.size());
    }
}