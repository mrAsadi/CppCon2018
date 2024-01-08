#ifndef CPPCON2018_WEBSOCKET_SESSION_HPP
#define CPPCON2018_WEBSOCKET_SESSION_HPP

#include "net.hpp"
#include "beast.hpp"
#include "shared_state.hpp"
#include "include/jwt-cpp/traits/boost-json/defaults.h"
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
// Forward declaration
class shared_state;

/** Represents an active WebSocket connection to the server
 */
class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    beast::flat_buffer buffer_;
    websocket::stream<tcp::socket> ws_;
    std::shared_ptr<shared_state> state_;
    std::vector<std::shared_ptr<std::string const>> queue_;
    std::string connection_id;

    void fail(error_code ec, char const *what);
    void on_accept(error_code ec);
    void on_read(error_code ec, std::size_t bytes_transferred);
    void on_write(error_code ec, std::size_t bytes_transferred);

public:
    websocket_session(
        tcp::socket socket,
        std::shared_ptr<shared_state> const &state);

    ~websocket_session();

    template <class Body, class Allocator>
    void
    run(http::request<Body, http::basic_fields<Allocator>> req);

    // Send a message
    void
    send(std::shared_ptr<std::string const> const &ss);

private:
    std::string
    generate_random_string(int length);

    std::string
    url_decode(const std::string &input);
};

template <class Body, class Allocator>
void websocket_session::
    run(http::request<Body, http::basic_fields<Allocator>> req)
{
    // Accept the websocket handshake
    ws_.async_accept(
        req,
        std::bind(
            &websocket_session::on_accept,
            shared_from_this(),
            std::placeholders::_1));
    // Extract the token from the request's URI
    std::string token;
    if (req.target().find("?token=") != std::string::npos)
    {
        // Extract token from URI
        token = req.target().substr(req.target().find("?token=") + 7);
        // Decode the URL-encoded token
        token = url_decode(token);
    }
    const auto decoded_token = jwt::decode<jwt::traits::boost_json>(token);
    const auto verify = jwt::verify<jwt::traits::boost_json>().allow_algorithm(jwt::algorithm::hs256{"secret"}).with_issuer("auth0");
    verify.verify(decoded_token);
    
}

#endif
