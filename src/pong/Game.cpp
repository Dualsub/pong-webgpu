#include "pong/Game.h"

#include "pong/Application.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include <iostream>
#include <stdio.h>

namespace pong
{
    void Game::Initialize(Renderer &renderer)
    {
        std::cout << "Initializing game" << std::endl;
        m_tableModel = renderer.CreateModel("./dist/table.dat");
        m_paddelModel = renderer.CreateModel("./dist/racket.dat");
        m_ballModel = renderer.CreateModel("./dist/ball.dat");
        m_debugPlane = renderer.CreateQuad({1.0f, 1.0f}, {1.0f, 0.0f, 0.0f});
        Connection &connection = Application::GetConnection();
        connection.Initialize(0);
    }

    float Game::CalculateBallHeight(glm::vec2 position, glm::vec2 velocity)
    {
        const float c_g = 9.82f;
        const float vx = velocity.x;
        const bool isMovingRight = velocity.x > 0.0f;
        const bool hasBounced = isMovingRight ? position.x > c_arenaWidth * c_tabelHitLocation : position.x < c_arenaWidth * (1.0f - c_tabelHitLocation);

        const float xorigin = hasBounced ? c_arenaWidth * c_tabelHitLocation : 0.0f;
        const float x0 = (isMovingRight ? xorigin : c_arenaWidth - xorigin);
        const float xtarget = hasBounced ? c_arenaWidth : c_arenaWidth * c_tabelHitLocation;
        const float x1 = (isMovingRight ? xtarget : c_arenaWidth - xtarget);
        const float x = position.x;

        const float y0 = hasBounced ? 0.0f : c_padelTableHitOffset;
        const float y1 = hasBounced ? c_padelTableHitOffset : 0.0f;

        const float vy = -(vx * (y1 - y0 + (c_g * std::pow(x0 - x1, 2)) / (2 * std::pow(vx, 2)))) / (x0 - x1);
        const float py = y0 + (vy * (x - x0)) / vx - (c_g * std::pow(x - x0, 2)) / (2 * std::pow(vx, 2));

        return py;
    }

    void Game::Update(float deltaTime)
    {
        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Connection &connection = Application::GetConnection();
        GameStateMessage *msg = connection.GetLatestMessage();

        if (msg == nullptr)
        {
            return;
        }

        // Update ball
        float ballHeight = CalculateBallHeight(msg->ball.position * c_scaleFactor, msg->ball.velocity * c_scaleFactor);
        m_ball.transform.position = glm::vec3(msg->ball.position.x * c_scaleFactor.x, ballHeight, msg->ball.position.y * c_scaleFactor.y);
        for (uint32_t i = 0; i < msg->players.size(); i++)
        {
            if (m_players.size() <= i)
            {
                m_players.push_back({});
            }

            m_players[i].transform.position = glm::vec3(msg->players[i].position.x * c_scaleFactor.x, c_padelTableHitOffset, msg->players[i].position.y * c_scaleFactor.y);
        }
    }

    void Game::Render(Renderer &renderer)
    {
        static glm::mat4 renderTransformOffset = glm::translate(glm::mat4(1.0f), glm::vec3(c_padelWidth / 2.0f, 0.0f, c_padelHeight / 2.0f)) * glm::mat4_cast(glm::quat(glm::vec3(0.0f, glm::radians(90.0f), glm::radians(90.0f))));
        renderer.SetCameraView(m_camera.transform.GetMatrix());

        std::vector<glm::mat4> playerTransforms = std::vector<glm::mat4>(m_players.size());
        for (uint32_t i = 0; i < playerTransforms.size(); i++)
        {
            playerTransforms[i] = m_players[i].transform.GetMatrix() * renderTransformOffset;
        }

        renderer.SubmitInstances(m_paddelModel.get(), playerTransforms);
        renderer.SubmitInstances(m_ballModel.get(), {m_ball.transform.GetMatrix()});

        renderer.SubmitInstances(m_tableModel.get(), {m_table.transform.GetMatrix()});

        // // For debugging, translate and scale
        // glm::mat4 planeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(c_arenaWidth / 2.0f, 0.0f, c_arenaHeight / 2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(c_arenaWidth, 1.0f, c_arenaHeight));
        // glm::mat4 paddleTransform1 = glm::translate(glm::mat4(1.0f), m_players[0].transform.position - glm::vec3(-c_padelWidth / 2.0f, 0.0f, -c_padelHeight / 2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(c_padelWidth, 1.0f, c_padelHeight));
        // glm::mat4 paddleTransform2 = glm::translate(glm::mat4(1.0f), m_players[1].transform.position - glm::vec3(-c_padelWidth / 2.0f, 0.0f, -c_padelHeight / 2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(c_padelWidth, 1.0f, c_padelHeight));
        // renderer.SubmitInstances(m_debugPlane.get(), {planeTransform});
    }

    void Game::Terminate()
    {
    }

}