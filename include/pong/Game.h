#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

namespace pong
{
    // Component types
    struct CTransform
    {
        glm::vec3 position;
        glm::quat rotation;

        CTransform() = default;
        CTransform(const glm::vec3 &position, const glm::quat &rotation)
            : position(position), rotation(rotation) {}

        glm::mat4 GetMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
        }
    };

    // Entity types
    struct EPlayer
    {
        CTransform transform;
    };

    struct EBall
    {
        CTransform transform;
    };

    class Game
    {
    private:
        std::vector<EPlayer> m_players;
        EBall m_ball;

    public:
        Game() = default;
        ~Game() = default;
    };
}