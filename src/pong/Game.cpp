#include "pong/Game.h"

#include "pong/Renderer.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include <iostream>
#include <stdio.h>

namespace pong
{
    void Game::Initialize(Renderer &renderer)
    {
        m_tableModel = renderer.CreateModel("./dist/table.dat");
        m_paddelModel = renderer.CreateModel("./dist/racket.dat");
        m_ballModel = renderer.CreateModel("./dist/ball.dat");
        m_debugPlane = renderer.CreateQuad({800.0f * c_scaleFactor.x, 600.0f * c_scaleFactor.y}, {1.0f, 0.0f, 0.0f});
        m_connection.Initialize(0);
    }

    void Game::Update(float deltaTime)
    {
        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        GameStateMessage *msg = m_connection.GetLatestMessage();
        if (msg == nullptr)
        {
            return;
        }

        m_ball.transform.position = glm::vec3(msg->ball.position.x * c_scaleFactor.x, 0.0f, msg->ball.position.y * c_scaleFactor.y);

        for (uint32_t i = 0; i < msg->players.size(); i++)
        {
            if (m_players.size() <= i)
            {
                m_players.push_back({});
            }

            m_players[i].transform.position = glm::vec3(msg->players[i].position.x * c_scaleFactor.x, 0.0f, msg->players[i].position.y * c_scaleFactor.y);
        }
    }

    void Game::Render(Renderer &renderer)
    {
        renderer.SetCameraView(m_camera.transform.GetMatrix());

        std::vector<glm::mat4> playerTransforms = std::vector<glm::mat4>(m_players.size());
        for (uint32_t i = 0; i < playerTransforms.size(); i++)
        {
            playerTransforms[i] = m_players[i].transform.GetMatrix();
        }

        renderer.SubmitInstances(m_paddelModel.get(), playerTransforms);
        renderer.SubmitInstances(m_ballModel.get(), {m_ball.transform.GetMatrix()});

        renderer.SubmitInstances(m_tableModel.get(), {m_table.transform.GetMatrix()});

        // // For debugging
        // renderer.SubmitInstances(m_debugPlane.get(), {glm::translate(glm::mat4(1.0f), glm::vec3(800.0f * c_scaleFactor.x / 2.0f, 0.0f, 600.0f * c_scaleFactor.y / 2.0f))});
    }

    void Game::Terminate()
    {
    }

}