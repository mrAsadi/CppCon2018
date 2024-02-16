#include "listener.hpp"
#include "shared_state.hpp"
#include <iostream>
#include "common/server_certificate.hpp"
#include <thread>
#include <boost/smart_ptr.hpp>

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage: advanced-server-flex <address> <port> <doc_root> \n" <<
            "Example:\n" <<
            "    advanced-server-flex 0.0.0.0 8080 . \n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = argv[3];
    

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv13};
    // This holds the self-signed certificate used by the server
    setup_ssl_context(ctx);

    // Create and launch a listening port
    boost::make_shared<listener>(
        ioc,
        ctx,
        tcp::endpoint{address, port},
        boost::make_shared<shared_state>(doc_root))->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](boost::system::error_code const&, int)
        {
            // Stop the io_context. This will cause run()
            // to return immediately, eventually destroying the
            // io_context and any remaining handlers in it.
            ioc.stop();
        });

    // Run the I/O service on the main thread
    ioc.run();
    // (If we get here, it means we got a SIGINT or SIGTERM)
    return EXIT_SUCCESS;
}
