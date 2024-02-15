#include "http_session.hpp"
#include "websocket_session.hpp"
#include <iostream>

// Return a reasonable mime type based on the extension of a file.
boost::beast::string_view
mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if (pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm"))
        return "text/html";
    if (iequals(ext, ".html"))
        return "text/html";
    if (iequals(ext, ".php"))
        return "text/html";
    if (iequals(ext, ".css"))
        return "text/css";
    if (iequals(ext, ".txt"))
        return "text/plain";
    if (iequals(ext, ".js"))
        return "application/javascript";
    if (iequals(ext, ".json"))
        return "application/json";
    if (iequals(ext, ".xml"))
        return "application/xml";
    if (iequals(ext, ".swf"))
        return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))
        return "video/x-flv";
    if (iequals(ext, ".png"))
        return "image/png";
    if (iequals(ext, ".jpe"))
        return "image/jpeg";
    if (iequals(ext, ".jpeg"))
        return "image/jpeg";
    if (iequals(ext, ".jpg"))
        return "image/jpeg";
    if (iequals(ext, ".gif"))
        return "image/gif";
    if (iequals(ext, ".bmp"))
        return "image/bmp";
    if (iequals(ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff"))
        return "image/tiff";
    if (iequals(ext, ".tif"))
        return "image/tiff";
    if (iequals(ext, ".svg"))
        return "image/svg+xml";
    if (iequals(ext, ".svgz"))
        return "image/svg+xml";
    return "application/text";
}

std::string
path_cat(
    boost::beast::string_view base,
    boost::beast::string_view path)
{
    if (base.empty())
        return path;
    std::string result = base;
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto &c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <class Body, class Allocator>
http::message_generator handle_request(
    boost::beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>> &&req)
{

    if (req.target() == "/api/ws" &&
        req.method() == http::verb::get)
    {

        // json::object claims{
        //     {"sub", "user123"},
        //     {"iss", "your_issuer"},
        //     {"aud", "your_audience"},
        //     {"exp", std::time(nullptr) + 3600} // Token expiration time
        // };

        const auto token = jwt::create<jwt::traits::boost_json>()
                               .set_issuer("auth0")
                               .set_audience("aud0")
                               .set_issued_at(std::chrono::system_clock::now())
                               .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
                               .sign(jwt::algorithm::hs256{"secret"});

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = boost::json::serialize(token); // Convert the JSON object to a string
        res.prepare_payload();
        return res;
    }

    // Returns a bad request response
    auto const bad_request =
        [&req](boost::beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "bad request :" + boost::string_view(why).to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
        [&req](boost::beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "not founded :" + boost::string_view(target).to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
        [&req](boost::beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "server error :" + boost::string_view(what).to_string();
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return bad_request("Illegal request-target");

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
        return not_found(req.target());

    // Handle an unknown error
    if (ec)
        return server_error(ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

//------------------------------------------------------------------------------

http_session::
    http_session(
        tcp::socket &&socket,
        ssl::context &ctx,
        std::shared_ptr<shared_state> const &state)
    : stream_(std::move(socket), ctx),state_(state)
{
}

void http_session::
    run()
{
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    // Note, this is the buffered version of the handshake.
    net::dispatch(
            stream_.get_executor(),
            beast::bind_front_handler(
                &http_session::on_run,
                shared_from_this()));
}

void http_session::
    on_run()
{
    // Set the timeout.
    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(30));

    // Perform the SSL handshake
    stream_.async_handshake(
        ssl::stream_base::server,
        beast::bind_front_handler(
            &http_session::on_handshake,
            shared_from_this()));
}

void http_session::
    on_handshake(beast::error_code ec)
{
    if (ec)
        return fail(ec, "handshake");

    do_read();
}

void http_session::
    do_read()
{
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout.
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream_, buffer_, req_,
                     beast::bind_front_handler(
                         &http_session::on_read,
                         shared_from_this()));
}

void http_session::
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec)
        return fail(ec, "read");

    if (websocket::is_upgrade(req_))
    {
        // Create a WebSocket session by transferring the socket
        std::make_shared<websocket_session>(
            std::move(release_stream()), state_)
            ->run(std::move(req_));
        return;
    }

    send_response(handle_request(state_->doc_root(), std::move(req_)));
}
beast::ssl_stream<beast::tcp_stream> http_session::
    release_stream()
{
    return std::move(stream_);
}

void http_session::
    send_response(http::message_generator &&msg)
{
    bool keep_alive = msg.keep_alive();

    // Write the response
    beast::async_write(
        stream_,
        std::move(msg),
        beast::bind_front_handler(
            &http_session::on_write,
            this->shared_from_this(),
            keep_alive));
}

void http_session::
    on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    if (!keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    }

    // Read another request
    do_read();
}

void http_session::
    do_close()
{
    // Set the timeout.
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Perform the SSL shutdown
    stream_.async_shutdown(
        beast::bind_front_handler(
            &http_session::on_shutdown,
            shared_from_this()));
}

void http_session::
    on_shutdown(beast::error_code ec)
{
    if (ec)
        return fail(ec, "shutdown");

    // At this point the connection is closed gracefully
}

// Report a failure
void http_session::
    fail(beast::error_code ec, char const *what)
{
    // Don't report on canceled operations
    if (ec == net::error::operation_aborted)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}