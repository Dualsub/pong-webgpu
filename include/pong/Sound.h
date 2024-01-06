#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace pong
{
    class Sound
    {
    private:
        uint32_t bufferId = 0;
        uint32_t sourceId = 0;

    public:
        Sound() = default;
        Sound(uint32_t bufferId, uint32_t sourceId)
            : bufferId(bufferId), sourceId(sourceId) {}
        ~Sound();

        Sound(const Sound &) = delete;
        Sound &operator=(const Sound &) = delete;

        void Play();
        void PlayAt(const glm::vec3 &position, float pitch = 1.0f);

        static std::unique_ptr<Sound> Create(const std::string &path);
    };
}