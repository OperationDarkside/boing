#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

// This function handles a single HTTP session
net::awaitable<void> do_session(tcp::socket socket) {
    beast::flat_buffer buffer;
    try {
        for (;;) {
            // Read a request
            http::request<http::string_body> req;
            co_await http::async_read(socket, buffer, req, net::use_awaitable);

            // Set up the response
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Hello from WSL! Modern C++ is running.";
            res.prepare_payload();

            // Send the response
            co_await http::async_write(socket, res, net::use_awaitable);

            if (res.need_eof()) {
                break;
            }
        }
    } catch (std::exception const& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
    
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

// This function listens for incoming connections
net::awaitable<void> do_listen(tcp::endpoint endpoint) {
    auto executor = co_await net::this_coro::executor;
    tcp::acceptor acceptor(executor, endpoint);

    for (;;) {
        tcp::socket socket = co_await acceptor.async_accept(net::use_awaitable);
        // Spawn a new coroutine for each connection
        net::co_spawn(executor, do_session(std::move(socket)), net::detached);
    }
}

int main() {
    try {
        unsigned short port = 8080;
        // Bind to 0.0.0.0 so it's visible outside WSL
        auto const address = net::ip::make_address("0.0.0.0");

        net::io_context ioc{1};
        
        net::co_spawn(ioc, do_listen({address, port}), net::detached);

        std::cout << "Server starting on http://localhost:" << port << std::endl;
        ioc.run();
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}