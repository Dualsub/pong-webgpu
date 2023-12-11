#include "pong/Game.h"

#include "pong/Renderer.h"

namespace pong
{
    void Game::Initialize(Renderer &renderer)
    {
        m_tableModel = renderer.CreateModel("./dist/pong.dat");
    }

    void Game::Update(float deltaTime)
    {
        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        m_table.transform.SetMatrix(glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
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