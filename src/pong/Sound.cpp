#include "pong/Sound.h"

#include "AL/al.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

namespace pong
{

    struct WavHeader
    {
        char riff[4]; // "RIFF"
        uint32_t size;
        char wave[4]; // "WAVE"
        char fmt[4];  // "fmt "
        uint32_t fmtSize;
        uint16_t format;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        char data[4]; // "data"
        uint32_t dataSize;
    };

    std::unique_ptr<Sound> Sound::Create(const std::string &path)
    {
        WavHeader header;
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Failed to open .wav-file: " << path << std::endl;
            return nullptr;
        }

        file.read(reinterpret_cast<char *>(&header), sizeof(WavHeader));

        if (std::strncmp(header.riff, "RIFF", 4) || std::strncmp(header.wave, "WAVE", 4))
        {
            std::cerr << "Invalid .wav-file: " << path << std::endl;
            return nullptr;
        }

        ALenum format;
        if (header.bitsPerSample == 16)
        {
            format = (header.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        }
        else
        {
            format = (header.channels == 2) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        }

        std::vector<char> data(header.dataSize);
        file.read(data.data(), data.size());
        file.close();

        uint32_t bufId = 0;
        alGenBuffers(1, &bufId);
        alBufferData(bufId, format, data.data(), data.size(), header.sampleRate);

        uint32_t srcId = 0;
        alGenSources(1, &srcId);
        alSourceQueueBuffers(srcId, 1, &bufId);

        return std::make_unique<Sound>(bufId, srcId);
    }

    Sound::~Sound()
    {
        alDeleteSources(1, &sourceId);
        alDeleteBuffers(1, &bufferId);
    }

    void Sound::Play()
    {
        alSourcePlay(sourceId);
    }

    void Sound::PlayAt(const glm::vec3 &position, float pitch)
    {
        alSourcef(sourceId, AL_PITCH, pitch);
        alSource3f(sourceId, AL_POSITION, position.x, position.y, position.z);
        alSourcePlay(sourceId);
    }
}