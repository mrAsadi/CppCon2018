#include "websocket_session.hpp"

websocket_session::
    websocket_session(
        beast::ssl_stream<beast::tcp_stream> &&stream,
        boost::shared_ptr<shared_state> const &state)
    : ws_(std::move(stream)), state_(state)
{
}
websocket_session::
    ~websocket_session()
{
    // Remove this session from the list of active sessions
    state_->disconnect(connection_id);
}

void websocket_session::
    fail(beast::error_code ec, char const *what)
{
    // Don't report these
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}

void websocket_session::
    on_accept(beast::error_code ec)
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
            beast::error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::
    on_close(beast::error_code ec)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "close");
}

void websocket_session::
    on_read(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "read");

    // Send to all connections
    // state_->broadcast(beast::buffers_to_string(buffer_.data()));

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Read another message
    ws_.async_read(
        buffer_,
        [sp = shared_from_this()](
            beast::error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::
    send(boost::shared_ptr<std::string const> const &ss)
{
    std::cout << "before writing" << ss << std::endl;
    queue_.push_back(ss);

    // Are we already writing?
    if (queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        [sp = shared_from_this()](
            beast::error_code ec, std::size_t bytes)
        {
            sp->on_write(ec, bytes);
        });
    std::cout << "after writing" << ss << std::endl;
}
void websocket_session::
    on_write_401(beast::error_code ec, std::size_t)
{
    if (ec)
        return fail(ec, "write");
}
void websocket_session::
    on_write(beast::error_code ec, std::size_t)
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
                beast::error_code ec, std::size_t bytes)
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

std::string
websocket_session::url_decode(const std::string &input)
{
    std::string result;
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == '%' && i + 2 < input.size() &&
            std::isxdigit(input[i + 1]) && std::isxdigit(input[i + 2]))
        {
            result += static_cast<char>(std::stoi(input.substr(i + 1, 2), 0, 16));
            i += 2;
        }
        else if (input[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

void websocket_session::close_with_401(http::request<http::string_body> &req, const std::string &error_message)
{
    // Close the WebSocket connection
    ws_.async_close(websocket::close_code::normal,
        std::bind(
            &websocket_session::on_close,
            shared_from_this(),
            std::placeholders::_1));

    // Send an HTTP response with a 401 status code and an error message
    http::response<http::string_body> res{http::status::unauthorized, req.version()};
    res.set(http::field::server, "ir-websocket-server");
    res.set(http::field::content_type, "application/json");
    res.body() = "Unauthorized: " + error_message;
    res.prepare_payload();

    using response_type = typename std::decay<decltype(res)>::type;
    auto sp = boost::make_shared<response_type>(std::forward<decltype(res)>(res));

    http::async_write(ws_.next_layer() , *sp,
        [self = shared_from_this(), sp](
            beast::error_code ec, std::size_t bytes)
        {
            self->on_write_401(ec, bytes);
        });
}