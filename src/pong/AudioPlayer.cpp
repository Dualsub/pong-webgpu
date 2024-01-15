#include "pong/AudioPlayer.h"

namespace pong
{
    bool AudioPlayer::Initialize()
    {
        device = alcOpenDevice(nullptr);
        if (!device)
        {
            return false;
        }

        context = alcCreateContext(device, nullptr);
        if (!context)
        {
            return false;
        }

        alcMakeContextCurrent(context);

        alListenerf(AL_GAIN, c_globalVolume);

        return true;
    }

    void AudioPlayer::Terminate()
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    void AudioPlayer::SetListenerPosition(const glm::vec3 &position)
    {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
    }
}