#pragma once

#include "pong/Sound.h"

#include "AL/al.h"
#include "AL/alc.h"
#include <glm/glm.hpp>

#include <memory>

namespace pong
{
    class AudioPlayer
    {
    private:
        const float c_globalVolume = 0.1f;

        ALCdevice *device = nullptr;
        ALCcontext *context = nullptr;

    public:
        bool Initialize();
        void Terminate();

        void SetListenerPosition(const glm::vec3 &position);
    };
}