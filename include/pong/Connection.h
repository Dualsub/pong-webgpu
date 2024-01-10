#pragma once

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <glm/glm.hpp>

#include <vector>
#include <cstdint>

namespace pong
{
    struct Head
    {
        uint32_t playerId = 0;
        uint32_t sequenceNumber = 0;
    };

    struct Player
    {
        int32_t playerId = 0;
        int32_t score = 0;
        glm::vec2 position = {};
    };

    struct Ball
    {
        glm::vec2 position = {};
        glm::vec2 velocity = {};
    };

    struct Events
    {
        bool hasHit = false;
        bool playerWasHit = false;
        bool hasSmashed = false;
        bool newRound = false;
    };

    struct GameStateMessage
    {
        Head head;
        std::vector<Player> players;
        Ball ball;
        Events events;
    };

    struct InputMessage
    {
        bool upPressed = false;
        bool downPressed = false;
        uint32_t sequenceNumber = 0;
        uint64_t timestamp = 0;
    };

    struct InputState
    {
        bool upPressed = false;
        bool downPressed = false;
    };

    class Connection
    {
    private:
        const uint32_t c_maxMessages = 10;

        EMSCRIPTEN_WEBSOCKET_T m_socket;

        uint32_t m_sequenceNumber = 0;
        std::mutex m_mutex;
        std::vector<GameStateMessage> m_messages;

        InputState m_inputState;

    public:
        Connection() = default;
        ~Connection() = default;

        void Initialize();

        GameStateMessage ParseMessage(const uint8_t *message, size_t length);
        std::vector<GameStateMessage> GetMessages() const { return m_messages; }
        GameStateMessage *GetLatestMessage() const { return m_messages.size() > 0 ? const_cast<GameStateMessage *>(&m_messages.back()) : nullptr; }
        GameStateMessage *PopLatestMessage();
        void AddMessage(const GameStateMessage &message);

        void SendInput();

        void SendPressedUp(bool pressed);
        void SendPressedDown(bool pressed);
    };
}