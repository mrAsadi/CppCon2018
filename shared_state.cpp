//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "shared_state.hpp"
#include "websocket_session.hpp"

shared_state::
    shared_state(std::string doc_root)
    : doc_root_(std::move(doc_root))
{
}

void shared_state::
    connect(const std::string &connection_id, websocket_session *session)
{
    sessions_[connection_id] = session;
    send(connection_id, "you connected as :" + connection_id);
}

void shared_state::
    disconnect(const std::string &connection_id)
{
    if (connection_id != "")
    {
        sessions_.erase(connection_id);
        boost::beast::flat_buffer buff;
        std::string myString = "a client disconnected :" + connection_id;
        boost::beast::ostream(buff) << myString;
        broadcast(beast::buffers_to_string(buff.data()));
    }
}

void shared_state::
    send(const std::string &connection_id, const std::string &message)
{
    auto const session = get(connection_id);
    if (session != nullptr)
    {
        auto const ss = std::make_shared<std::string const>(std::move(message));
        session->send(ss);
    }
}

void shared_state::
    broadcast(const std::string &message)
{
    for (const auto &entry : sessions_)
    {
        websocket_session *session = entry.second;
        if (session != nullptr)
        {
            auto const ss = std::make_shared<std::string const>(std::move(message));
            session->send(ss);
        }
    }
}

websocket_session *shared_state::
    get(const std::string &connection_id)
{
    auto it = sessions_.find(connection_id);
    return (it != sessions_.end()) ? it->second : nullptr;
}
