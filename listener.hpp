#ifndef IR_WEBSOCKET_SERVER_LISTENER_HPP
#define IR_WEBSOCKET_SERVER_LISTENER_HPP

#include "beast.hpp"
#include <memory>
#include <string>

// Forward declaration
class shared_state;

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
    std::shared_ptr<shared_state> state_;

    void fail(error_code ec, char const* what);
    void on_accept(beast::error_code ec, tcp::socket socket);
    void do_accept();

public:
    listener(
        net::io_context& ioc,
        ssl::context& ctx,
        tcp::endpoint endpoint,
        std::shared_ptr<shared_state> const& state);

    // Start accepting incoming connections
    void run();
};

#endif
