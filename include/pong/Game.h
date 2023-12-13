#pragma once

#include "pong/Connection.h"
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

        CTransform(const glm::mat4 &matrix)
        {
            SetMatrix(matrix);
        }

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

    struct ECamera
    {
        CTransform transform;
    };

    class Game
    {
    private:
        // Scaling will effect the physics, ball and players will be squished
        const glm::vec2 c_scaleFactor = glm::vec2(1.0f / 3.95f, 1.0f / 3.95f);
        const float c_arenaWidth = 800.0f * c_scaleFactor.x;
        const float c_arenaHeight = 600.0f * c_scaleFactor.y;
        Connection m_connection;

        std::vector<EPlayer> m_players;
        EBall m_ball;
        ETable m_table = {{glm::vec3(c_arenaWidth / 2.0f, -79.0f, c_arenaHeight / 2.0f), glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f))}};
        ECamera m_camera = {{glm::lookAt(glm::vec3(c_arenaWidth / 2.0f, 300.0f, -c_arenaHeight / 2.0f),
                                         glm::vec3(c_arenaWidth / 2.0f, 0.0f, c_arenaHeight / 2.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f))}};

        // Graphics
        std::unique_ptr<Model> m_ballModel;
        std::unique_ptr<Model> m_paddelModel;
        std::unique_ptr<Model> m_tableModel;
        std::unique_ptr<Model> m_debugPlane;

    public:
        Game() = default;
        ~Game() = default;

        void Initialize(class Renderer &renderer);
        void Update(float deltaTime);
        void Render(class Renderer &renderer);
        void Terminate();
    };
}