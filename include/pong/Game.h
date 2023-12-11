#pragma once

#include "pong/Model.h"

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

        void SetMatrix(const glm::mat4 &matrix)
        {
            position = glm::vec3(matrix[3]);
            rotation = glm::quat_cast(matrix);
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

    struct ETable
    {
        CTransform transform;
    };

    class Game
    {
    private:
        std::vector<EPlayer> m_players;
        EBall m_ball;
        ETable m_table;

        // Graphics
        std::unique_ptr<Model> m_ballModel;
        std::unique_ptr<Model> m_paddelModel;
        std::unique_ptr<Model> m_tableModel;

    public:
        Game() = default;
        ~Game() = default;

        void Initialize(class Renderer &renderer);
        void Update(float deltaTime);
        void Render(class Renderer &renderer);
        void Terminate();
    };
}