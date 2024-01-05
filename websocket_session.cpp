//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "websocket_session.hpp"

websocket_session::
    websocket_session(
        tcp::socket socket,
        std::shared_ptr<shared_state> const &state)
    : ws_(std::move(socket)), state_(state)
{
}

websocket_session::
    ~websocket_session()
{
    // Remove this session from the list of active sessions
    state_->disconnect(connection_id);
}

void websocket_session::
    fail(error_code ec, char const *what)
{
    // Don't report these
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}

void websocket_session::
    on_accept(error_code ec)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "accept");

    connection_id = generate_random_string(16);
    state_->connect(connection_id, this);

    // Read a message
    ws_.async_read(
        buffer_,
        [sp = shared_from_this()](
            error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::
    on_read(error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "read");

    // Send to all connections
    state_->broadcast(beast::buffers_to_string(buffer_.data()));

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Read another message
    ws_.async_read(
        buffer_,
        [sp = shared_from_this()](
            error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::
    send(std::shared_ptr<std::string const> const &ss)
{
    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if (queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        [sp = shared_from_this()](
            error_code ec, std::size_t bytes)
        {
            sp->on_write(ec, bytes);
        });
}

void websocket_session::
    on_write(error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if (!queue_.empty())
        ws_.async_write(
            net::buffer(*queue_.front()),
            [sp = shared_from_this()](
                error_code ec, std::size_t bytes)
            {
                sp->on_write(ec, bytes);
            });
}

std::string
websocket_session::generate_random_string(int length)
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int charset_size = sizeof(charset) - 1;

    std::mt19937_64 rng(std::time(nullptr));
    std::uniform_int_distribution<int> distribution(0, charset_size - 1);

    std::string random_string;
    random_string.reserve(length);

    for (int i = 0; i < length; ++i)
    {
        random_string.push_back(charset[distribution(rng)]);
    }

    return random_string;
}
