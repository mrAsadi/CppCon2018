#ifndef IR_WEBSOCKET_SERVER_BEAST_HPP
#define IR_WEBSOCKET_SERVER_BEAST_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace ssl = boost::asio::ssl; 
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace json = boost::json;
namespace net = boost::asio;
using error_code = beast::error_code; 
using tcp = boost::asio::ip::tcp;

#endif
