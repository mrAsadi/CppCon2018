//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef IR_WEBSOCKET_SERVER_SHARED_STATE_HPP
#define IR_WEBSOCKET_SERVER_SHARED_STATE_HPP

#include <memory>
#include <string>
#include <unordered_map>

// Forward declaration
class websocket_session;

// Represents the shared server state
class shared_state
{
    std::string doc_root_;

    // This simple method of tracking
    // sessions only works with an implicit
    // strand (i.e. a single-threaded server)
    std::unordered_map<std::string, websocket_session *> sessions_;

public:
    explicit shared_state(std::string doc_root);

    std::string const &
    doc_root() const noexcept
    {
        return doc_root_;
    }

    void connect(const std::string &connection_id,websocket_session* session);
    void disconnect(const std::string &connection_id);
    void send(const std::string &connection_id,const std::string &message);
    void broadcast(const std::string &message);
    websocket_session* get(const std::string &connection_id);
};

#endif
