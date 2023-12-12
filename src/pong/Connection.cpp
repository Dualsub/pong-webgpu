#include "pong/Connection.h"

#include <iostream>

namespace pong
{
    EM_BOOL onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData)
    {
        Connection *connection = reinterpret_cast<Connection *>(userData);
        std::cout << "onopen" << std::endl;
        return EM_TRUE;
    }
    EM_BOOL onerror(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData)
    {
        Connection *connection = reinterpret_cast<Connection *>(userData);
        std::cerr << "onerror" << std::endl;

        return EM_TRUE;
    }

    EM_BOOL onclose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData)
    {
        Connection *connection = reinterpret_cast<Connection *>(userData);
        std::cout << "onclose: " << websocketEvent->wasClean << std::endl;
        return EM_TRUE;
    }

    EM_BOOL onmessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData)
    {
        Connection *connection = reinterpret_cast<Connection *>(userData);
        uint8_t *current = const_cast<uint8_t *>(websocketEvent->data);
        GameStateMessage msg = connection->ParseMessage(current, websocketEvent->numBytes);
        connection->AddMessage(msg);

        return EM_TRUE;
    }

    void Connection::Initialize(uint32_t gameId)
    {
        if (!emscripten_websocket_is_supported())
        {
            std::cerr << "WebSockets are not supported." << std::endl;
            exit(1);
        }

        std::string url = "wss://www.kurskollen.se/play?id=" + std::to_string(gameId);

        EmscriptenWebSocketCreateAttributes wsAttrs = {
            url.c_str(),
            NULL,
            EM_TRUE};

        m_socket = emscripten_websocket_new(&wsAttrs);
        emscripten_websocket_set_onopen_callback(m_socket, this, onopen);
        emscripten_websocket_set_onerror_callback(m_socket, this, onerror);
        emscripten_websocket_set_onclose_callback(m_socket, this, onclose);
        emscripten_websocket_set_onmessage_callback(m_socket, this, onmessage);
    }

    GameStateMessage Connection::ParseMessage(const uint8_t *message, size_t length)
    {
        const size_t numPlayers = (length - (4 + 4 + 8 + 3)) / 16;

        GameStateMessage msg;
        uint8_t *current = const_cast<uint8_t *>(message);

        Head *head = reinterpret_cast<Head *>(current);
        msg.head = *head;
        current += sizeof(Head);

        std::vector<Player> players;
        for (uint32_t i = 0; i < numPlayers; ++i)
        {
            Player *player = reinterpret_cast<Player *>(current);
            players.push_back(*player);
            current += sizeof(Player);
        }
        msg.players = players;

        Ball *ball = reinterpret_cast<Ball *>(current);
        msg.ball = *ball;
        current += sizeof(Ball);

        Events *events = reinterpret_cast<Events *>(current);
        msg.events = *events;
        current += sizeof(Events);

        return msg;
    }

    void Connection::AddMessage(const GameStateMessage &message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back(message);

        if (m_messages.size() > c_maxMessages)
        {
            m_messages.erase(m_messages.begin());
        }
    }

    void Connection::SendInput(bool upPressed, bool downPressed)
    {
        InputMessage message;
        message.upPressed = upPressed;
        message.downPressed = downPressed;
        message.sequenceNumber = m_sequenceNumber++;
        auto now = std::chrono::steady_clock::now();
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        emscripten_websocket_send_binary(m_socket, &message, sizeof(InputMessage));
    }
}