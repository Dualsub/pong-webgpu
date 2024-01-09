#pragma once

#include "pong/Connection.h"
#include "pong/Model.h"
#include "pong/Texture.h"
#include "pong/Sound.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <vector>
#include <algorithm>
#include <random>

namespace pong
{
    // Scaling will effect the physics, ball and players will be squished
    static constexpr glm::vec2 c_scaleFactor = glm::vec2(1.0f / 3.95f, 1.0f / 3.95f);
    static constexpr float c_arenaWidth = 800.0f * c_scaleFactor.x;
    static constexpr float c_arenaHeight = 600.0f * c_scaleFactor.y;

    static constexpr float c_padelWidth = 10.0f * c_scaleFactor.x;
    static constexpr float c_padelHeight = 100.0f * c_scaleFactor.y;

    static constexpr float c_ballRadius = 10.0f * c_scaleFactor.x;

    // Constants for 3D movement
    static constexpr float c_padelTableHitOffset = 20.0f;
    static constexpr float c_tabelHitLocation = 0.75f;

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
        int32_t id = 0;
        float targetAngle = 90.0f;
        float currentAngle = 0.0f;
        uint32_t score = 0;
        CTransform transform;
    };

    struct EBall
    {
        CTransform transform;
        glm::vec3 velocity;
    };

    struct ETable
    {
        CTransform transform;
    };

    struct ECamera
    {
        CTransform transform;
        float trauma = 0.0f;
        float traumaDecay = 1.5f;
        glm::mat4 offset = glm::mat4(1.0f);
    };

    class Game
    {
    private:
        // Entities
        std::map<int32_t, EPlayer>
            m_players;
        EBall m_ball;
        ETable m_table = {{glm::vec3(c_arenaWidth / 2.0f, -79.0f, c_arenaHeight / 2.0f), glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f))}};
        ECamera m_camera = {{glm::lookAt(glm::vec3(c_arenaWidth / 2.0f, 250.0f, 2 * c_arenaHeight / 2.0f),
                                         glm::vec3(c_arenaWidth / 2.0f, 0.0f, c_arenaHeight / 2.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f))}};

        // Graphics
        std::unique_ptr<Model> m_ballModel;
        std::unique_ptr<Model> m_paddelModel;
        std::unique_ptr<Model> m_tableModel;
        std::unique_ptr<Model> m_debugPlane;
        std::unique_ptr<Texture> m_numbersTextureAtlas;

        std::unique_ptr<Sound> m_hitSound;
        std::unique_ptr<Sound> m_smashSound;
        std::unique_ptr<Sound> m_racketSound;
        std::unique_ptr<Sound> m_winSound;
        std::unique_ptr<Sound> m_loseSound;

        float CalculateBallHeight(glm::vec2 position, glm::vec2 velocity);
        bool HasBallHitTable(glm::vec2 position, glm::vec2 velocity);

    public:
        Game() = default;
        ~Game() = default;

        void Initialize(class Renderer &renderer);
        void Update(float deltaTime);
        void Render(class Renderer &renderer);
        void Terminate();
    };
}