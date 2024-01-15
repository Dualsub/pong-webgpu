#include "pong/Game.h"

#include "pong/Application.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include <iostream>
#include <random>
#include <set>
#include <stdio.h>

namespace pong
{
    void Game::Initialize(Renderer &renderer)
    {
        std::cout << "Initializing game" << std::endl;

        m_tableModel = renderer.CreateModel("./dist/table.dat");
        m_paddelModel = renderer.CreateModel("./dist/racket.dat");
        m_ballModel = renderer.CreateModel("./dist/ball.dat");
        // m_debugPlane = renderer.CreateQuad({1.0f, 1.0f}, glm::vec3(156, 72, 72) * 1.0f / 255.0f);

        m_numbersTextureAtlas = renderer.CreateTexture("./dist/numbers.dat");

        m_hitSound = Sound::Create("./dist/ball_hit_1.wav");
        m_smashSound = Sound::Create("./dist/smash_hit.wav");
        m_racketSound = Sound::Create("./dist/racket_hit.wav");
        m_winSound = Sound::Create("./dist/win.wav");
        m_loseSound = Sound::Create("./dist/lose.wav");
        Application::GetAudioPlayer().SetListenerPosition(m_camera.transform.position);

        Connection &connection = Application::GetConnection();
        connection.Initialize();

        m_camera.offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1000.0f, -c_arenaHeight / 2.0f));
    }

    float Game::CalculateBallHeight(glm::vec2 position, glm::vec2 velocity)
    {
        const float c_lowerTarget = c_ballRadius;
        const float c_g = 9.82f;
        const float vx = velocity.x;
        const bool isMovingRight = velocity.x > 0.0f;
        const bool hasBounced = isMovingRight ? position.x > c_arenaWidth * c_tabelHitLocation : position.x < c_arenaWidth * (1.0f - c_tabelHitLocation);

        const float xorigin = hasBounced ? c_arenaWidth * c_tabelHitLocation : 0.0f;
        const float x0 = (isMovingRight ? xorigin : c_arenaWidth - xorigin);
        const float xtarget = hasBounced ? c_arenaWidth : c_arenaWidth * c_tabelHitLocation;
        const float x1 = (isMovingRight ? xtarget : c_arenaWidth - xtarget);
        const float x = position.x;

        const float y0 = hasBounced ? c_lowerTarget : c_padelTableHitOffset;
        const float y1 = hasBounced ? c_padelTableHitOffset : c_lowerTarget;

        const float vy = -(vx * (y1 - y0 + (c_g * std::pow(x0 - x1, 2)) / (2 * std::pow(vx, 2)))) / (x0 - x1);
        const float py = y0 + (vy * (x - x0)) / vx - (c_g * std::pow(x - x0, 2)) / (2 * std::pow(vx, 2));

        return py;
    }

    bool Game::HasBallHitTable(glm::vec2 position, glm::vec2 velocity)
    {
        static bool lastHasBounced = false; // Ugly, but we only have one ball
        const bool isMovingRight = velocity.x > 0.0f;
        const bool hasBounced = isMovingRight ? position.x > c_arenaWidth * c_tabelHitLocation : position.x < c_arenaWidth * (1.0f - c_tabelHitLocation);
        bool value = hasBounced && !lastHasBounced;
        lastHasBounced = hasBounced;
        return value;
    }

    void Game::PositionScoreInstances(std::vector<SpriteBatch::Instance> &instances, uint32_t score, glm::vec3 origin)
    {
        const float letterWidth = 148.0f;
        const float letterWorldWidth = letterWidth * 0.1f;
        const float letterSpacing = -4.0f;

        std::string scoreText = std::to_string(score);
        instances.resize(instances.size() + scoreText.size());

        glm::vec3 start = glm::vec3(-(scoreText.size() * letterWorldWidth + letterSpacing * (scoreText.size() - 1)) / 2.0f, 0.0f, 0.0f);
        for (uint32_t i = 0; i < scoreText.size(); i++)
        {
            SpriteBatch::Instance instance;
            char c = scoreText[i];
            uint32_t number = c - '0';

            glm::vec3 offset = glm::vec3(i * (letterWorldWidth + letterSpacing), 0.0f, 0.0f);
            glm::vec3 position = (start + offset) + origin;
            instance.transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(letterWorldWidth, 1.0f, -letterWorldWidth));
            instance.offsetAndSize = glm::vec4(number * letterWidth / m_numbersTextureAtlas->GetWidth(), 0.0f, letterWidth / m_numbersTextureAtlas->GetWidth(), 1.0f);

            instances[i + instances.size() - scoreText.size()] = instance;
        }
    }

    void Game::Update(float deltaTime)
    {
        static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Connection &connection = Application::GetConnection();
        GameStateMessage *msg = connection.GetLatestMessage();

        if (msg == nullptr || msg->handeled)
        {
            return;
        }

        static std::mt19937 gen(0);
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        // Update ball
        glm::vec2 ballVelocity = msg->ball.velocity * c_scaleFactor;
        glm::vec2 ballPosition = msg->ball.position * c_scaleFactor;
        float ballHeight = CalculateBallHeight(ballPosition, ballVelocity);
        m_ball.transform.position = glm::vec3(ballPosition.x, ballHeight, ballPosition.y);
        m_ball.velocity = glm::vec3(ballVelocity.x, 0.0f, ballVelocity.y);
        msg->events.hasHit = msg->events.hasHit || HasBallHitTable(ballPosition, ballVelocity);

        std::set<int32_t> playersToRemove;
        for (auto &&[id, player] : m_players)
        {
            playersToRemove.insert(id);
        }

        // Update players
        for (auto &&msgPlayer : msg->players)
        {
            playersToRemove.erase(msgPlayer.playerId);

            if (!m_players.contains(msgPlayer.playerId))
            {
                m_players[msgPlayer.playerId] = {};
            }

            EPlayer &player = m_players[msgPlayer.playerId];
            glm::vec3 newPlayerPos = glm::vec3(msgPlayer.position.x * c_scaleFactor.x, c_padelTableHitOffset, msgPlayer.position.y * c_scaleFactor.y);

            // Check if player has higher score
            if (msgPlayer.score > player.score)
            {
                if (msgPlayer.playerId == msg->head.playerId)
                {
                    m_winSound->PlayAt(m_ball.transform.position, 250.0f);
                }
                else
                {
                    m_loseSound->PlayAt(m_ball.transform.position, 250.0f);
                }
            }

            player.score = msgPlayer.score;

            if (glm::abs(newPlayerPos.z - player.transform.position.z) > glm::epsilon<float>())
            {
                float targetAngle = 0.0f;
                bool isOrientedUp = newPlayerPos.z > player.transform.position.z;
                targetAngle = isOrientedUp ? -75.0f : 75.0f;
                player.targetAngle = targetAngle;
            }

            player.currentAngle += (player.targetAngle - player.currentAngle) * 5.0f * deltaTime;
            player.transform.position = newPlayerPos;
        }

        for (auto &&id : playersToRemove)
        {
            m_players.erase(id);
        }

        // Update camera and play hit sounds
        if (msg->events.hasSmashed)
        {
            m_camera.trauma = 0.6f;
            m_smashSound->PlayAt(m_ball.transform.position);
        }
        else if (msg->events.playerWasHit)
        {
            m_camera.trauma = 0.3f;
            m_racketSound->PlayAt(m_ball.transform.position);
        }
        else if (msg->events.hasHit)
        {
            // m_camera.trauma = 0.0f;
            float pitch = 1.0f + (glm::abs(dist(gen)) * 0.15f);
            m_hitSound->PlayAt(m_ball.transform.position, 1.0f, pitch);
        }
        else
        {
            m_camera.trauma -= deltaTime * m_camera.traumaDecay;
            m_camera.trauma = glm::max(m_camera.trauma, 0.0f);
        }

        float shake = m_camera.trauma * m_camera.trauma;

        const float maxShake = 10.0f;
        glm::mat4 newOffset =
            glm::rotate(glm::mat4(1.0f), glm::radians(maxShake * dist(gen) * shake), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(maxShake * dist(gen) * shake), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::vec3 ballTranslation = glm::vec3(m_ball.transform.position.x - c_arenaWidth / 2.0f, 0.0f, m_ball.transform.position.z - c_arenaHeight / 2.0f);
        newOffset = glm::translate(newOffset, -ballTranslation * 0.05f);

        // Asymtotically approach target
        m_camera.offset = glm::mix(m_camera.offset, newOffset, 5.0f * deltaTime);

        msg->handeled = true;
    }

    void Game::Render(Renderer &renderer)
    {
        static glm::mat4 paddelRenderTransformOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-c_padelWidth / 2.0f, 0.0f, c_padelHeight / 2.0f));
        static glm::mat4 ballRenderTransformOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-c_ballRadius, 0.0f, -c_ballRadius)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.3f));
        renderer.SetCameraView(m_camera.transform.GetMatrix() * m_camera.offset);

        std::vector<glm::mat4> playerTransforms = std::vector<glm::mat4>(m_players.size());
        std::vector<SpriteBatch::Instance> scoreInstances = std::vector<SpriteBatch::Instance>(m_players.size());
        uint32_t i = 0;
        const float letterWidth = 148.0f;
        for (const auto &[id, player] : m_players)
        {
            // Player
            float angle = player.currentAngle;
            playerTransforms[i] = player.transform.GetMatrix() * paddelRenderTransformOffset * glm::mat4_cast(glm::quat(glm::vec3(0.0f, glm::radians(angle), glm::radians(90.0f))));

            // Score
            float xOffset = (player.transform.position.x < c_arenaWidth / 2.0f ? -1.0f : 1.0f) * c_arenaWidth / 4.0f;
            PositionScoreInstances(scoreInstances, player.score, glm::vec3(c_arenaWidth / 2.0f + xOffset, 0.0f, c_arenaHeight / 6.0f));

            i++;
        }

        renderer.SubmitInstances(m_numbersTextureAtlas.get(), scoreInstances);
        renderer.SubmitInstances(m_paddelModel.get(), playerTransforms);
        renderer.SubmitInstances(m_tableModel.get(), {m_table.transform.GetMatrix()});
        renderer.SubmitInstances(m_ballModel.get(), {m_ball.transform.GetMatrix() * ballRenderTransformOffset});

        // Floor
        // SpriteBatch::Instance floorInstance;
        // floorInstance.transform = glm::translate(glm::mat4(1.0f), glm::vec3(c_arenaWidth / 2.0f, -80.0f, c_arenaHeight / 2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(c_arenaWidth * 10.0f, 1.0f, c_arenaWidth * 10.0f));
        // renderer.SubmitInstances(m_floorSprite.get(), {floorInstance});
        // renderer.SubmitInstances(m_debugPlane.get(), {floorInstance.transform});
    }

    void Game::Terminate()
    {
    }

}