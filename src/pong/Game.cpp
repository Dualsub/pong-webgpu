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
        m_tableModel = renderer.CreateModel("./dist/pong.dat");
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

        m_table.transform.position = glm::vec3(msg->ball.position.x / 10.0f, 0.0f, msg->ball.position.y / 10.0f);
    }

    void Game::Render(Renderer &renderer)
    {
        // std::vector<glm::mat4> playerTransforms = std::vector<glm::mat4>(m_players.size());
        // for (uint32_t i = 0; i < playerTransforms.size(); i++)
        // {
        //     playerTransforms[i] = m_players[i].transform.GetMatrix();
        // }

        // renderer.SubmitInstances(m_paddelModel.get(), playerTransforms);
        // renderer.SubmitInstances(m_ballModel.get(), {m_ball.transform.GetMatrix()});

        renderer.SubmitInstances(m_tableModel.get(), {m_table.transform.GetMatrix()});
    }

    void Game::Terminate()
    {
    }

}