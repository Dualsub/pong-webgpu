#pragma once

#include <webgpu/webgpu_cpp.h>

#include <string>
#include <memory>

namespace pong
{
    class Texture
    {
    private:
        static uint32_t s_nextId;

        uint32_t m_id = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;

        wgpu::Texture m_texture = {};
        wgpu::TextureView m_textureView = {};
        wgpu::Sampler m_sampler = {};

    public:
        Texture() = default;
        Texture(uint32_t id, uint32_t width, uint32_t height, wgpu::Texture texture, wgpu::TextureView textureView, wgpu::Sampler sampler)
            : m_id(id), m_width(width), m_height(height), m_texture(texture), m_textureView(textureView), m_sampler(sampler) {}
        ~Texture() = default;

        uint32_t GetId() const { return m_id; }
        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }
        wgpu::Texture GetTexture() const { return m_texture; }
        wgpu::TextureView GetTextureView() const { return m_textureView; }
        wgpu::Sampler GetSampler() const { return m_sampler; }

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;
        static std::unique_ptr<Texture> Create(const wgpu::Device &device, const wgpu::Queue &queue, const std::string &path);
    };

}