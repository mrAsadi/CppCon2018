//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef IR_WEBSOCKET_SERVER_HTTP_SESSION_HPP
#define IR_WEBSOCKET_SERVER_HTTP_SESSION_HPP

#include "beast.hpp"
#include "include/jwt-cpp/traits/boost-json/defaults.h"
#include "shared_state.hpp"
#include <cstdlib>
#include <memory>

/** Represents an established HTTP connection
*/
class http_session : public std::enable_shared_from_this<http_session>
{
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<shared_state> state_;
    http::request<http::string_body> req_;
    void fail(beast::error_code ec, char const* what);

public:
    http_session(
        tcp::socket &&socket,
        ssl::context& ctx,
        std::shared_ptr<shared_state> const& state);

    beast::ssl_stream<beast::tcp_stream> release_stream();
    void run();
    void on_run();
    void on_handshake(beast::error_code ec);
    void on_shutdown(beast::error_code ec);
    void do_close();
    void do_read();
    void on_write(bool keep_alive,beast::error_code ec,std::size_t bytes_transferred);
    void send_response(http::message_generator&& msg);
    void on_read(beast::error_code ec,std::size_t bytes_transferred);
};

#endif
