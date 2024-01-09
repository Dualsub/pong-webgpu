#include "pong/Texture.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <iostream>
#include <vector>

namespace pong
{
    uint32_t Texture::s_nextId = 1;

    struct TextureHeader
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t numChannels = 0;
        uint32_t bytesPerChannel = 0;
    };

    std::unique_ptr<Texture> Texture::Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path)
    {
        std::fstream file(path, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            return nullptr;
        }

        TextureHeader header;
        file.read(reinterpret_cast<char *>(&header), sizeof(TextureHeader));

        if (header.bytesPerChannel != 1)
        {
            std::cerr << "Unsupported texture format: " << path << std::endl;
            return nullptr;
        }

        std::cout << "width: " << header.width << std::endl;
        std::cout << "height: " << header.height << std::endl;
        std::cout << "numChannels: " << header.numChannels << std::endl;
        std::cout << "bytesPerChannel: " << header.bytesPerChannel << std::endl;

        std::vector<uint8_t> data(header.width * header.height * header.numChannels);
        file.read(reinterpret_cast<char *>(data.data()), data.size());
        file.close();

        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = header.width;
        descriptor.size.height = header.height;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;

        wgpu::Texture texture = device.CreateTexture(&descriptor);

        wgpu::ImageCopyTexture imageCopyTexture;
        imageCopyTexture.texture = texture;
        imageCopyTexture.mipLevel = 0;
        imageCopyTexture.origin = {0, 0, 0};
        imageCopyTexture.aspect = wgpu::TextureAspect::All;

        wgpu::TextureDataLayout source;
        source.offset = 0;
        source.bytesPerRow = header.numChannels * header.width;
        source.rowsPerImage = header.height;

        queue.WriteTexture(&imageCopyTexture, data.data(), header.width * header.height * header.numChannels * header.bytesPerChannel, &source, &descriptor.size);

        wgpu::TextureViewDescriptor viewDescriptor;
        viewDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;

        wgpu::TextureView textureView = texture.CreateView(&viewDescriptor);

        wgpu::SamplerDescriptor samplerDescriptor;
        samplerDescriptor.addressModeU = wgpu::AddressMode::Repeat;
        samplerDescriptor.addressModeV = wgpu::AddressMode::Repeat;
        samplerDescriptor.addressModeW = wgpu::AddressMode::Repeat;
        samplerDescriptor.minFilter = wgpu::FilterMode::Linear;
        samplerDescriptor.magFilter = wgpu::FilterMode::Linear;
        samplerDescriptor.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDescriptor.lodMinClamp = 0.0f;
        samplerDescriptor.lodMaxClamp = 0.0f;
        samplerDescriptor.compare = wgpu::CompareFunction::Undefined;

        wgpu::Sampler sampler = device.CreateSampler(&samplerDescriptor);

        uint32_t id = s_nextId++;

        return std::make_unique<Texture>(id, header.width, header.height, texture, textureView, sampler);
    }
}